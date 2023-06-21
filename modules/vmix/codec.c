#include <re_atomic.h>
#include <mix.h>
#include "vmix.h"


static videnc_encode_h *ench;
static videnc_update_h *encupdh;

static viddec_decode_h *dech;
static viddec_update_h *decupdh;


static int enc_update(struct videnc_state **vesp, const struct vidcodec *vc,
		      struct videnc_param *prm, const char *fmtp,
		      videnc_packet_h *pkth, void *arg)
{
	return encupdh(vesp, vc, prm, fmtp, pkth, arg);
}


static int dec_update(struct viddec_state **vdsp, const struct vidcodec *vc,
		      const char *fmtp)
{
	return decupdh(vdsp, vc, fmtp);
}


static int encode(struct videnc_state *ves, bool update,
		  const struct vidframe *frame, uint64_t timestamp)
{
	return ench(ves, update, frame, timestamp);
}


static int decode(struct viddec_state *vds, struct vidframe *frame,
		  bool *intra, bool marker, uint16_t seq, struct mbuf *mb)
{
	return dech(vds, frame, intra, marker, seq, mb);
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
