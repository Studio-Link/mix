#include <re_atomic.h>
#include <mix.h>
#include "baresip.h"
#include "vmix.h"


static videnc_encode_h *ench;
static videnc_update_h *encupdh;

struct videnc_state {
	struct videnc_state *vesp;
	videnc_packet_h *pkth;
	void *arg;
};

static struct {
	bool keyframe;
	bool marker;
	uint64_t rtp_ts;
	struct mbuf *hdr_mb;
	struct mbuf *pld_mb;
	uint64_t timestamp;
} p;

static bool keyframe_required = false;


static void destructor(void *arg)
{
	struct videnc_state *st = arg;

	mem_deref(st->vesp);
}


static int packeth(bool marker, uint64_t rtp_ts, const uint8_t *hdr,
		   size_t hdr_len, const uint8_t *pld, size_t pld_len,
		   void *arg)
{
	struct videnc_state *st = arg;

	p.marker = marker;
	p.rtp_ts = rtp_ts;

	mbuf_rewind(p.hdr_mb);
	mbuf_write_mem(p.hdr_mb, hdr, hdr_len);

	mbuf_rewind(p.pld_mb);
	mbuf_write_mem(p.pld_mb, pld, pld_len);

	return st->pkth(marker, rtp_ts, hdr, hdr_len, pld, pld_len, st->arg);
}


static int update(struct videnc_state **vesp, const struct vidcodec *vc,
		  struct videnc_param *prm, const char *fmtp,
		  videnc_packet_h *pkth, void *arg)
{

	struct videnc_state *st;
	int err;

	if (!vesp || !vc || !prm || !pkth)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	st->pkth = pkth;
	st->arg	 = arg;

	err = encupdh(&st->vesp, vc, prm, fmtp, packeth, st);

	if (err)
		mem_deref(st);
	else
		*vesp = st;

	return err;
}


static int encode(struct videnc_state *ves, bool update,
		  const struct vidframe *frame, uint64_t timestamp)
{
	if (timestamp == p.timestamp) {
		if (update && !p.keyframe) {
			keyframe_required = true;
			return 0;
		}

		return ves->pkth(p.marker, p.rtp_ts, p.hdr_mb->buf,
				 p.hdr_mb->end, p.pld_mb->buf, p.pld_mb->end,
				 ves->arg);
	}

	if (keyframe_required) {
		keyframe_required = false;
		update		  = true;
	}

	p.timestamp = timestamp;
	p.keyframe  = update;

	return ench(ves->vesp, update, frame, timestamp);
}


static struct vidcodec h264_1 = {
	.name	 = "H264",
	.variant = "packetization-mode=1",
	.encupdh = update,
	.ench	 = encode,
};


void vmix_codec_init(void)
{
	const struct vidcodec *v;

	v = vidcodec_find(baresip_vidcodecl(), "H264", "packetization-mode=1");
	if (!v) {
		warning("vmix_codec_init h264_1 failed\n");
		return;
	}

	encupdh = v->encupdh;
	ench	= v->ench;

	h264_1.decupdh	  = v->decupdh;
	h264_1.dech	  = v->dech;
	h264_1.fmtp_ench  = v->fmtp_ench;
	h264_1.fmtp_cmph  = v->fmtp_cmph;
	h264_1.packetizeh = v->packetizeh;

	list_clear(baresip_vidcodecl());
	vidcodec_register(baresip_vidcodecl(), &h264_1);

	p.hdr_mb = mbuf_alloc(32);
	p.pld_mb = mbuf_alloc(1024);
}


void vmix_codec_close(void)
{
	vidcodec_unregister(&h264_1);
	mem_deref(p.hdr_mb);
	mem_deref(p.pld_mb);
}
