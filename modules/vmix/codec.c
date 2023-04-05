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

struct vid_pkt {
	struct le le;
	bool marker;
	uint64_t rtp_ts;
	struct mbuf hdr_mb;
	struct mbuf pld_mb;
};

static struct {
	struct list vid_pktl;
	bool keyframe;
	uint64_t timestamp;
} c;

static bool keyframe_required = false;


static void destructor_pkt(void *arg)
{
	struct vid_pkt *pkt = arg;

	mem_deref(pkt->hdr_mb.buf);
	mem_deref(pkt->pld_mb.buf);
}


static void destructor_enc(void *arg)
{
	struct videnc_state *st = arg;

	mem_deref(st->vesp);
}


static int packeth(bool marker, uint64_t rtp_ts, const uint8_t *hdr,
		   size_t hdr_len, const uint8_t *pld, size_t pld_len,
		   void *arg)
{
	struct videnc_state *st = arg;

	struct vid_pkt *pkt;

	pkt = mem_zalloc(sizeof(struct vid_pkt), destructor_pkt);
	if (!pkt)
		return ENOMEM;

	pkt->marker = marker;
	pkt->rtp_ts = rtp_ts;

	mbuf_write_mem(&pkt->hdr_mb, hdr, hdr_len);
	mbuf_write_mem(&pkt->pld_mb, pld, pld_len);

	list_append(&c.vid_pktl, &pkt->le, pkt);

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

	st = mem_zalloc(sizeof(*st), destructor_enc);
	if (!st)
		return ENOMEM;

	st->pkth = pkth;
	st->arg	 = arg;

	/* TODO: can maybe optimize with single encupdh (active video call) */
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
	struct le *le;
	int err = 0;

	if (timestamp == c.timestamp) {
		if (update && !c.keyframe) {
			keyframe_required = true;
			return 0;
		}

		LIST_FOREACH(&c.vid_pktl, le)
		{
			struct vid_pkt *pkt = le->data;

			err |= ves->pkth(pkt->marker, pkt->rtp_ts,
					 pkt->hdr_mb.buf, pkt->hdr_mb.end,
					 pkt->pld_mb.buf, pkt->pld_mb.end,
					 ves->arg);
		}
		return err;
	}

	if (keyframe_required) {
		keyframe_required = false;
		update		  = true;
	}

	list_flush(&c.vid_pktl);

	c.timestamp = timestamp;
	c.keyframe  = update;

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
}


void vmix_codec_close(void)
{
	vidcodec_unregister(&h264_1);
}
