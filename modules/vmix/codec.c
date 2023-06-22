#include <re_atomic.h>
#include <mix.h>
#include "vmix.h"

/* H264 only */
static videnc_encode_h *ench;
static videnc_update_h *encupdh;
static viddec_decode_h *dech;
static viddec_update_h *decupdh;

struct videnc_state {
	videnc_packet_h *pkth;
	const struct video *vid;
	void *vesp; /* Sub-Codec state */
};

struct viddec_state {
	const struct video *vid;
	void *vdsp; /* Sub-Codec state */
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
}


static int enc_update(struct videnc_state **vesp, const struct vidcodec *vc,
		      struct videnc_param *prm, const char *fmtp,
		      videnc_packet_h *pkth, const struct video *vid)
{
	struct videnc_state *ves;

	ves = mem_zalloc(sizeof(struct videnc_state), videnc_deref);
	if (!ves)
		return ENOMEM;

	ves->vid = vid;

	*vesp = ves;

	return encupdh((struct videnc_state **)&ves->vesp, vc, prm, fmtp, pkth,
		       vid);
}


static int encode(struct videnc_state *ves, bool update,
		  const struct vidframe *frame, uint64_t timestamp)
{
	/* return ves->pkth(); */

#if 0
typedef int (videnc_packet_h)(bool marker, uint64_t rtp_ts,
			      const uint8_t *hdr, size_t hdr_len,
			      const uint8_t *pld, size_t pld_len,
			      void *arg);
#endif

	return ench(ves->vesp, update, frame, timestamp);
}


static int dec_update(struct viddec_state **vdsp, const struct vidcodec *vc,
		      const char *fmtp, const struct video *vid)
{
	struct viddec_state *vds;

	vds = mem_zalloc(sizeof(struct viddec_state), viddec_deref);
	if (!vds)
		return ENOMEM;

	vds->vid = vid;

	*vdsp = vds;

	return decupdh((struct viddec_state **)&vds->vdsp, vc, fmtp, vid);
}


static int decode(struct viddec_state *vds, struct vidframe *frame,
		  bool *intra, bool marker, uint16_t seq, uint64_t ts,
		  struct mbuf *mb)
{

	return dech(vds->vdsp, frame, intra, marker, seq, ts, mb);
}


static struct vidcodec h264_0 = {
	.name	 = "H264",
	.variant = "packetization-mode=0",
	.encupdh = enc_update,
	.ench	 = encode,
	.decupdh = dec_update,
	.dech	 = decode,
};


static struct vidcodec h264_1 = {
	.name	 = "H264",
	.variant = "packetization-mode=1",
	.encupdh = enc_update,
	.ench	 = encode,
	.decupdh = dec_update,
	.dech	 = decode,
};


void vmix_codec_init(void)
{
	const struct vidcodec *v;

	v = vidcodec_find(baresip_vidcodecl(), "H264", "packetization-mode=0");
	if (!v) {
		warning("vmix_codec_init h264_0 failed\n");
		return;
	}

	encupdh = v->encupdh;
	ench	= v->ench;

	decupdh = v->decupdh;
	dech	= v->dech;

	h264_0.fmtp_ench  = v->fmtp_ench;
	h264_0.fmtp_cmph  = v->fmtp_cmph;
	h264_0.packetizeh = v->packetizeh;

	v = vidcodec_find(baresip_vidcodecl(), "H264", "packetization-mode=1");
	if (!v) {
		warning("vmix_codec_init h264_1 failed\n");
		return;
	}

	h264_1.fmtp_ench  = v->fmtp_ench;
	h264_1.fmtp_cmph  = v->fmtp_cmph;
	h264_1.packetizeh = v->packetizeh;

	list_clear(baresip_vidcodecl());

	vidcodec_register(baresip_vidcodecl(), &h264_0);
	vidcodec_register(baresip_vidcodecl(), &h264_1);
}


void vmix_codec_close(void)
{
	vidcodec_unregister(&h264_0);
	vidcodec_unregister(&h264_1);
}
