#include <mix.h>
#include "baresip.h"
#include "vmix.h"

/* H264 only */
static videnc_encode_h *ench;
static videnc_update_h *encupdh;
static videnc_packetize_h *packetizeh;
static viddec_decode_h *dech;
static viddec_update_h *decupdh;

static struct hash *dec_list;
static mtx_t *dec_mtx;

enum {
	MAX_PKT_TIME = 50, /**< in [ms] */
};

struct videnc_state {
	videnc_packet_h *pkth;
	const struct video *vid;
	uint64_t last_ts;
	void *vesp; /* Sub-Codec state */
	bool is_pkt_src;
};

struct viddec_state {
	struct le le;
	const struct video *vid;
	void *vdsp; /* Sub-Codec state */
	struct list pktl;
	uint64_t last_id;
	mtx_t *mtx;
	const char *dev;
};

struct pkt {
	uint64_t id;
	struct le le;
	struct mbuf *mb;
	bool marker;
	uint64_t ts;
	uint64_t ts_eol;
};


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

	err = encupdh((struct videnc_state **)&ves->vesp, vc, prm, fmtp, pkth,
		      vid);
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
	return ench(ves->vesp, update, frame, timestamp);
}


static int packetize(struct videnc_state *ves, const struct vidpacket *vpkt)
{
	if (ves->is_pkt_src) {
		uint64_t ts = video_calc_rtp_timestamp_fix(vpkt->timestamp);
		return ves->pkth(vpkt->keyframe, ts, NULL, 0, vpkt->buf,
				 vpkt->size, ves->vid);
	}
	return packetizeh(ves->vesp, vpkt);
}


static int dec_update(struct viddec_state **vdsp, const struct vidcodec *vc,
		      const char *fmtp, const struct video *vid)
{
	struct viddec_state *vds;
	int err;

	vds = mem_zalloc(sizeof(struct viddec_state), viddec_deref);
	if (!vds)
		return ENOMEM;

	vds->dev = video_get_disp_dev(vid);
	vds->vid = vid;

	err = mutex_alloc(&vds->mtx);
	if (err) {
		mem_deref(vds);
		return err;
	}

	err = decupdh((struct viddec_state **)&vds->vdsp, vc, fmtp, vid);
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


static void pkt_deref(void *arg)
{
	struct pkt *pkt = arg;

	mem_deref(pkt->mb);
	list_unlink(&pkt->le);
}


static int decode(struct viddec_state *vds, struct vidframe *frame,
		  bool *intra, bool marker, uint16_t seq, uint64_t ts,
		  struct mbuf *mb)
{
	struct pkt *pkt = mem_zalloc(sizeof(struct pkt), pkt_deref);
	if (!pkt)
		return ENOMEM;

	pkt->mb	    = mbuf_dup(mb);
	pkt->marker = marker;
	pkt->ts	    = ts;
	pkt->ts_eol = ts + MAX_PKT_TIME * 1000;
	pkt->id	    = vds->last_id++;

	mtx_lock(vds->mtx);
	list_append(&vds->pktl, &pkt->le, pkt);

	/* delayed queue cleanup */
	struct le *le = vds->pktl.head;
	while (le) {
		pkt = le->data;

		le = le->next;

		/* FIXME: Keep SPS/PPS workaround */
		if (pkt->id < 200)
			continue;

		if (ts > pkt->ts_eol)
			mem_deref(pkt);
		else
			break;
	}
	mtx_unlock(vds->mtx);

	return dech(vds->vdsp, frame, intra, marker, seq, ts, mb);
}


static struct vidcodec h264_0 = {
	.name	    = "H264",
	.variant    = "packetization-mode=0",
	.encupdh    = enc_update,
	.ench	    = encode,
	.decupdh    = dec_update,
	.dech	    = decode,
	.packetizeh = packetize,
};


static struct vidcodec h264_1 = {
	.name	    = "H264",
	.variant    = "packetization-mode=1",
	.encupdh    = enc_update,
	.ench	    = encode,
	.decupdh    = dec_update,
	.dech	    = decode,
	.packetizeh = packetize,
};


static bool list_apply_handler(struct le *le, void *arg)
{
	struct vidsrc_st *st	 = arg;
	struct viddec_state *vds = le->data;
	struct pkt *pkt		 = NULL;

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


int vmix_codec_init(void)
{
	const struct vidcodec *v;
	int err;

	err = mutex_alloc(&dec_mtx);
	if (err)
		return err;

	v = vidcodec_find(baresip_vidcodecl(), "H264", "packetization-mode=0");
	if (!v) {
		warning("vmix_codec_init h264_0 failed\n");
		return EINVAL;
	}

	encupdh = v->encupdh;
	ench	= v->ench;

	decupdh = v->decupdh;
	dech	= v->dech;

	packetizeh = v->packetizeh;

	h264_0.fmtp_ench = v->fmtp_ench;
	h264_0.fmtp_cmph = v->fmtp_cmph;

	v = vidcodec_find(baresip_vidcodecl(), "H264", "packetization-mode=1");
	if (!v) {
		warning("vmix_codec_init h264_1 failed\n");
		return EINVAL;
	}

	h264_1.fmtp_ench = v->fmtp_ench;
	h264_1.fmtp_cmph = v->fmtp_cmph;

	list_clear(baresip_vidcodecl());

	vidcodec_register(baresip_vidcodecl(), &h264_0);
	vidcodec_register(baresip_vidcodecl(), &h264_1);

	return hash_alloc(&dec_list, 32);
}


void vmix_codec_close(void)
{
	vidcodec_unregister(&h264_0);
	vidcodec_unregister(&h264_1);

	dec_list = mem_deref(dec_list);
	dec_mtx	 = mem_deref(dec_mtx);
}
