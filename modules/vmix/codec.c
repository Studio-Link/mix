#include <mix.h>
#include "baresip.h"
#include "vmix.h"


struct vmix_proxy {
	struct le le;
	struct vidcodec codec;
	videnc_encode_h *ench;
	videnc_update_h *encupdh;
	videnc_packetize_h *packetizeh;
	viddec_decode_h *dech;
	viddec_update_h *decupdh;
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
	const struct vmix_proxy *p;
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
	const struct vmix_proxy *p;
};

struct viddec_state {
	struct le le;
	const struct video *vid;
	void *vdsp; /* Sub-Codec state */
	struct list pktl;
	uint64_t last_id;
	mtx_t *mtx;
	const char *dev;
	const struct vmix_proxy *p;
};

struct dec_pkt {
	uint64_t id;
	struct le le;
	struct mbuf *mb;
	bool marker;
	uint64_t ts;
	uint64_t ts_eol;
};


static const struct vmix_proxy *vmix_proxy_find(const char *name,
						const char *variant)
{
	struct le *le;

	LIST_FOREACH(&proxyl, le)
	{
		struct vmix_proxy *p = le->data;

		if (name && 0 != str_casecmp(name, p->codec.name))
			continue;

		if (variant && 0 != str_casecmp(variant, p->codec.variant))
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

	err = ves->p->encupdh((struct videnc_state **)&ves->vesp, vc, prm,
			      fmtp, enc_packet_h, (void *)ves->p);
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

	return ves->p->ench(ves->vesp, keyframe, frame, timestamp);
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

	err = vds->p->decupdh((struct viddec_state **)&vds->vdsp, vc, fmtp,
			      vid);
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

#if 0
static void dec_pkt_deref(void *arg)
{
	struct dec_pkt *pkt = arg;

	mem_deref(pkt->mb);
	list_unlink(&pkt->le);
}
#endif


static int decode(struct viddec_state *vds, struct vidframe *frame,
		  struct viddec_packet *vpkt)
{
	if (!vds || !frame || !vpkt || !vpkt->mb)
		return EINVAL;
#if 0
	struct dec_pkt *pkt =
		mem_zalloc(sizeof(struct dec_pkt), dec_pkt_deref);
	if (!pkt)
		return ENOMEM;

	pkt->mb	    = mbuf_dup(vpkt->mb);
	pkt->marker = vpkt->hdr->m;
	pkt->ts	    = vpkt->timestamp;
	pkt->ts_eol = pkt->ts + MAX_PKT_TIME * 1000;
	pkt->id	    = vds->last_id++;

	mtx_lock(vds->mtx);
	list_append(&vds->pktl, &pkt->le, pkt);

	/* delayed queue cleanup */
	struct le *le = vds->pktl.head;
	while (le) {
		pkt = le->data;

		le = le->next;

		if (pkt->ts > pkt->ts_eol)
			mem_deref(pkt);
		else
			break;
	}
	mtx_unlock(vds->mtx);
#endif
	return vds->p->dech(vds->vdsp, frame, vpkt);
}


static bool list_apply_handler(struct le *le, void *arg)
{
	struct vidsrc_st *st	 = arg;
	struct viddec_state *vds = le->data;
	struct dec_pkt *pkt	 = NULL;

	if (0 != str_cmp(vds->dev, st->device + sizeof("pktsrc") - 1))
		return false;

	mtx_lock(vds->mtx);
	struct le *ple = vds->pktl.head;
	while (ple) {
		pkt = ple->data;

		ple = ple->next;

		/* skip already send */
		if (st->last_pkt && st->last_pkt >= pkt->id)
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
	struct vmix_proxy *p;
	const struct vidcodec *v;

	p = mem_zalloc(sizeof(struct vmix_proxy), NULL);
	if (!p)
		return;

	/* Proxy functions */
	p->codec.encupdh    = enc_update;
	p->codec.ench	    = encode;
	p->codec.decupdh    = dec_update;
	p->codec.dech	    = decode;
	p->codec.packetizeh = packetize;

	v = vidcodec_find(baresip_vidcodecl(), name, variant);
	if (!v) {
		warning("vmix: proxy_codec_alloc find %s failed\n", name);
		return;
	}

	/* Orignal functions */
	p->codec.name	   = name;
	p->codec.variant   = variant;
	p->codec.fmtp_ench = v->fmtp_ench;
	p->codec.fmtp_cmph = v->fmtp_cmph;

	p->encupdh    = v->encupdh;
	p->ench	      = v->ench;
	p->decupdh    = v->decupdh;
	p->dech	      = v->dech;
	p->packetizeh = v->packetizeh;

	list_append(&proxyl, &p->le, p);
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
#endif
	proxy_codec_alloc("VP8", NULL);

	list_clear(baresip_vidcodecl());

	LIST_FOREACH(&proxyl, le)
	{
		struct vmix_proxy *p = le->data;
		vidcodec_register(baresip_vidcodecl(), &p->codec);
	}

	return 0;
}


void vmix_codec_close(void)
{
	struct le *le;

	LIST_FOREACH(&proxyl, le)
	{
		struct vmix_proxy *p = le->data;
		vidcodec_unregister(&p->codec);
	}

	list_flush(&proxyl);
	list_flush(&enc_pktl);

	dec_list = mem_deref(dec_list);
	dec_mtx	 = mem_deref(dec_mtx);
}
