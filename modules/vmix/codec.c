#include <mix.h>
#include "baresip.h"
#include "vmix.h"


struct vmix_vcproxy {
	struct vidcodec vc; /* Keep first, to keep vidcodec order */
	struct vidcodec *ovc;
	struct le ple;
};

static struct hash *dec_list;
static mtx_t *dec_mtx;
static struct list proxyl = LIST_INIT;

struct enc_pkt {
	struct le le;
	bool marker;
	uint64_t rtp_ts;
	struct mbuf *hdr;
	size_t hdr_len;
	struct mbuf *pld;
	size_t pld_len;
	const struct vmix_vcproxy *p;
};

static struct list enc_pktl	     = LIST_INIT;
static RE_ATOMIC bool last_keyframe  = false;
static RE_ATOMIC bool force_keyframe = false;

enum {
	MAX_PKT_TIME = 50, /**< in [ms] */
};

struct videnc_state {
	videnc_packet_h *pkth;
	const struct video *vid;
	uint64_t last_ts;
	void *vesp; /* Sub-Codec state */
	bool is_pkt_src;
	const struct vmix_vcproxy *p;
};

struct viddec_state {
	struct le le;
	const struct video *vid;
	void *vdsp; /* Sub-Codec state */
	struct list pktl;
	uint64_t last_id;
	mtx_t *mtx;
	const char *dev;
	const struct vmix_vcproxy *p;
};

struct dec_pkt {
	uint64_t id;
	struct le le;
	struct mbuf *mb;
	bool marker;
	uint64_t ts;
	uint64_t ts_eol;
};


static const struct vmix_vcproxy *vmix_proxy_find(const char *name,
						  const char *variant)
{
	struct le *le;

	LIST_FOREACH(&proxyl, le)
	{
		struct vmix_vcproxy *p = le->data;

		if (name && 0 != str_casecmp(name, p->vc.name))
			continue;

		if (variant && 0 != str_casecmp(variant, p->vc.variant))
			continue;

		return p;
	}

	return NULL;
}


static void videnc_deref(void *arg)
{
	struct videnc_state *ves = arg;

	mem_deref(ves->vesp);
}


static void viddec_deref(void *arg)
{
	struct viddec_state *vds = arg;

	mtx_lock(dec_mtx);
	hash_unlink(&vds->le);
	mtx_unlock(dec_mtx);

	mem_deref(vds->vdsp);

	mtx_lock(vds->mtx);
	list_flush(&vds->pktl);
	mtx_unlock(vds->mtx);
	mem_deref(vds->mtx);
}


static void enc_pkt_deref(void *arg)
{
	struct enc_pkt *pkt = arg;

	mem_deref(pkt->hdr);
	mem_deref(pkt->pld);
	list_unlink(&pkt->le);
}


static int fmtp_ench(struct mbuf *mb, const struct sdp_format *fmt, bool offer,
		     void *data)
{
	const struct vmix_vcproxy *p = data;

	return p->ovc->fmtp_ench(mb, fmt, offer, p->ovc);
}


static bool fmtp_cmph(const char *params1, const char *params2, void *data)
{

	const struct vmix_vcproxy *p = data;

	return p->ovc->fmtp_cmph(params1, params2, p->ovc);
}


/* Copy encoded and packetized codec */
static int enc_packet_h(bool marker, uint64_t rtp_ts, const uint8_t *hdr,
			size_t hdr_len, const uint8_t *pld, size_t pld_len,
			const struct video *vid)
{
	int err;

	if (!hdr || !pld)
		return EINVAL;

	struct enc_pkt *pkt =
		mem_zalloc(sizeof(struct enc_pkt), enc_pkt_deref);
	if (!pkt)
		return ENOMEM;

	pkt->marker = marker;
	pkt->rtp_ts = rtp_ts;
	pkt->p	    = (void *)vid;

	pkt->hdr = mbuf_alloc(hdr_len);
	pkt->pld = mbuf_alloc(pld_len);

	err = mbuf_write_mem(pkt->hdr, hdr, hdr_len);
	err |= mbuf_write_mem(pkt->pld, pld, pld_len);

	if (err)
		return err;

	mbuf_set_pos(pkt->hdr, 0);
	mbuf_set_pos(pkt->pld, 0);

	list_append(&enc_pktl, &pkt->le, pkt);

	return 0;
}


static int enc_update(struct videnc_state **vesp, const struct vidcodec *vc,
		      struct videnc_param *prm, const char *fmtp,
		      videnc_packet_h *pkth, const struct video *vid)
{
	struct videnc_state *ves;
	const struct vmix_vcproxy *p = (struct vmix_vcproxy *)vc;
	int err;

	ves = mem_zalloc(sizeof(struct videnc_state), videnc_deref);
	if (!ves)
		return ENOMEM;

	const char *dev = video_get_src_dev(vid);
	ves->is_pkt_src = str_ncmp("pktsrc", dev, sizeof("pktsrc") - 1) == 0;

	ves->vid  = vid;
	ves->pkth = pkth;

	ves->p = vmix_proxy_find(vc->name, vc->variant);
	if (!ves->p) {
		mem_deref(ves);
		return EINVAL;
	}

	err = ves->p->ovc->encupdh((struct videnc_state **)&ves->vesp, p->ovc,
				   prm, fmtp, enc_packet_h, (void *)ves->p);
	if (err) {
		mem_deref(ves);
		return err;
	}

	*vesp = ves;

	return 0;
}


static int encode(struct videnc_state *ves, bool update,
		  const struct vidframe *frame, uint64_t timestamp)
{
	bool keyframe = update;

	if (re_atomic_rlx(&force_keyframe)) {
		keyframe = true;
		re_atomic_rlx_set(&force_keyframe, false);
	}

	re_atomic_rlx_set(&last_keyframe, keyframe);

	return ves->p->ovc->ench(ves->vesp, keyframe, frame, timestamp);
}


static int packetize(struct videnc_state *ves, const struct vidpacket *vpkt)
{

	if (ves->is_pkt_src) {
		uint64_t ts = video_calc_rtp_timestamp_fix(vpkt->timestamp);
		return ves->pkth(vpkt->keyframe, ts, NULL, 0, vpkt->buf,
				 vpkt->size, ves->vid);
	}

	struct le *le;
	LIST_FOREACH(&enc_pktl, le)
	{
		struct enc_pkt *p = le->data;
		if (ves->p != p->p)
			continue;

		int err = ves->pkth(p->marker, p->rtp_ts, mbuf_buf(p->hdr),
				    mbuf_get_left(p->hdr), mbuf_buf(p->pld),
				    mbuf_get_left(p->pld), ves->vid);
		if (err)
			return err;
	}

	return 0;
}


static int dec_update(struct viddec_state **vdsp, const struct vidcodec *vc,
		      const char *fmtp, const struct video *vid)
{
	struct viddec_state *vds;
	const struct vmix_vcproxy *p = (struct vmix_vcproxy *)vc;
	int err;

	vds = mem_zalloc(sizeof(struct viddec_state), NULL);
	if (!vds)
		return ENOMEM;

	err = mutex_alloc(&vds->mtx);
	if (err) {
		mem_deref(vds);
		return err;
	}

	mem_destructor(vds, viddec_deref);

	vds->dev = video_get_disp_dev(vid);
	vds->vid = vid;

	vds->p = vmix_proxy_find(vc->name, vc->variant);
	if (!vds->p) {
		mem_deref(vds);
		return EINVAL;
	}

	err = vds->p->ovc->decupdh((struct viddec_state **)&vds->vdsp, p->ovc,
				   fmtp, vid);
	if (err) {
		mem_deref(vds);
		return err;
	}

	mtx_lock(dec_mtx);
	hash_append(dec_list, (uint32_t)(uintptr_t)vid, &vds->le, vds);
	mtx_unlock(dec_mtx);

	*vdsp = vds;

	return 0;
}


static void dec_pkt_deref(void *arg)
{
	struct dec_pkt *pkt = arg;

	mem_deref(pkt->mb);
	list_unlink(&pkt->le);
}


static int decode(struct viddec_state *vds, struct vidframe *frame,
		  struct viddec_packet *vpkt)
{
	if (!vds || !frame || !vpkt || !vpkt->mb)
		return EINVAL;

	struct dec_pkt *pkt =
		mem_zalloc(sizeof(struct dec_pkt), dec_pkt_deref);
	if (!pkt)
		return ENOMEM;

	pkt->mb	    = mbuf_dup(vpkt->mb);
	pkt->marker = vpkt->hdr->m;
	pkt->ts	    = vpkt->timestamp;
	pkt->ts_eol = pkt->ts + MAX_PKT_TIME * 1000;
	pkt->id	    = ++vds->last_id;

	mtx_lock(vds->mtx);
	list_append(&vds->pktl, &pkt->le, pkt);

	/* delayed queue cleanup */
	struct le *le = vds->pktl.head;
	while (le) {
		pkt = le->data;

		le = le->next;

		if (vpkt->timestamp > pkt->ts_eol)
			mem_deref(pkt);
		else
			break;
	}
	mtx_unlock(vds->mtx);

	return vds->p->ovc->dech(vds->vdsp, frame, vpkt);
}


static bool list_apply_handler(struct le *le, void *arg)
{
	struct vidsrc_st *st	 = arg;
	struct viddec_state *vds = le->data;

	if (0 != str_cmp(vds->dev, st->device + sizeof("pktsrc") - 1))
		return false;

	mtx_lock(vds->mtx);
	struct le *ple = vds->pktl.head;
	while (ple) {
		struct dec_pkt *pkt = ple->data;

		ple = ple->next;

		/* skip already send */
		if (st->last_pkt >= pkt->id)
			continue;

		struct vidpacket packet = {.buf	      = mbuf_buf(pkt->mb),
					   .size      = mbuf_get_left(pkt->mb),
					   .timestamp = pkt->ts,
					   .keyframe  = pkt->marker};

		st->packeth(&packet, st->arg);

		st->last_pkt = pkt->id;
	}
	mtx_unlock(vds->mtx);

	return true;
}


/* called by pktsrc_thread */
void vmix_codec_pkt(struct vidsrc_st *st)
{
	mtx_lock(dec_mtx);
	(void)hash_apply(dec_list, list_apply_handler, st);
	mtx_unlock(dec_mtx);
}


bool vmix_last_keyframe(void)
{
	return re_atomic_rlx(&last_keyframe);
}


void vmix_request_keyframe(void)
{
	re_atomic_rlx_set(&force_keyframe, true);
}


void vmix_encode_flush(void)
{

	list_flush(&enc_pktl);
}


static void proxy_codec_alloc(const char *name, const char *variant)
{
	struct vmix_vcproxy *p;
	const struct vidcodec *v;

	v = vidcodec_find(baresip_vidcodecl(), name, variant);
	if (!v) {
		warning("vmix: proxy_codec_alloc find %s failed\n", name);
		return;
	}

	p = mem_zalloc(sizeof(struct vmix_vcproxy), NULL);
	if (!p)
		return;


	/* Proxy functions */
	p->vc.name    = name;
	p->vc.variant = variant;
	p->vc.encupdh = enc_update;
	p->vc.ench    = encode;
	p->vc.decupdh = dec_update;
	p->vc.dech    = decode;
	if (v->fmtp_ench)
		p->vc.fmtp_ench = fmtp_ench;
	if (v->fmtp_cmph)
		p->vc.fmtp_cmph = fmtp_cmph;
	p->vc.packetizeh = packetize;

	/* Orignal functions */
	p->ovc = (void *)v;

	list_append(&proxyl, &p->ple, p);
}


int vmix_codec_init(void)
{
	struct le *le;
	int err;

	err = mutex_alloc(&dec_mtx);
	if (err)
		return err;

	err = hash_alloc(&dec_list, 32);
	if (err)
		return err;

#if 0
	proxy_codec_alloc("H264", "packetization-mode=0");
	proxy_codec_alloc("H264", "packetization-mode=1");
#else
	proxy_codec_alloc("VP8", NULL);
#endif

	list_clear(baresip_vidcodecl());

	LIST_FOREACH(&proxyl, le)
	{
		struct vmix_vcproxy *p = le->data;
		vidcodec_register(baresip_vidcodecl(), (struct vidcodec *)p);
	}

	return 0;
}


void vmix_codec_close(void)
{
	struct le *le;

	LIST_FOREACH(&proxyl, le)
	{
		struct vmix_vcproxy *p = le->data;
		vidcodec_unregister((struct vidcodec *)p);
	}

	list_flush(&proxyl);
	list_flush(&enc_pktl);

	dec_list = mem_deref(dec_list);
	dec_mtx	 = mem_deref(dec_mtx);
}
