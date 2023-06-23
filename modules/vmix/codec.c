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

enum {
	MAX_PKT_TIME = 250, /**< in [ms] */
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
};

struct pkt {
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

	mem_deref(vds->vdsp);
	hash_unlink(&vds->le);
	list_flush(&vds->pktl);
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
	ves->is_pkt_src = str_ncmp("vmix_src", dev, 7) == 0;

	ves->vid = vid;

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
#if 0

#else
	return ench(ves->vesp, update, frame, timestamp);
#endif
}


static int packetize(struct videnc_state *ves, const struct vidpacket *packet)
{
#if 0
	if (ves->is_pkt_src) {
		ves->pkth();
typedef int (videnc_packet_h)(bool marker, uint64_t rtp_ts,
			      const uint8_t *hdr, size_t hdr_len,
			      const uint8_t *pld, size_t pld_len,
			      void *arg);
	}
#endif
	return packetizeh(ves->vesp, packet);
}


static int dec_update(struct viddec_state **vdsp, const struct vidcodec *vc,
		      const char *fmtp, const struct video *vid)
{
	struct viddec_state *vds;
	int err;

	vds = mem_zalloc(sizeof(struct viddec_state), viddec_deref);
	if (!vds)
		return ENOMEM;

	vds->vid = vid;

	err = decupdh((struct viddec_state **)&vds->vdsp, vc, fmtp, vid);
	if (err) {
		mem_deref(vds);
		return err;
	}

	hash_append(dec_list, (uint32_t)(uintptr_t)vid, &vds->le, vds);
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

	pkt->mb	    = mem_ref(mb);
	pkt->marker = marker;
	pkt->ts	    = ts;
	pkt->ts_eol = ts + MAX_PKT_TIME * 1000;

	list_append(&vds->pktl, &pkt->le, pkt);

	/* delayed queue cleanup */
	struct le *le = vds->pktl.head;
	while (le) {
		pkt = le->data;

		le = le->next;

		if (ts > pkt->ts_eol)
			mem_deref(pkt);
		else
			break;
	}

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


int vmix_codec_init(void)
{
	const struct vidcodec *v;

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
}
