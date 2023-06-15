#include <mix.h>

static struct ua *sip_ua;

static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	struct mix *mix = arg;

	(void)ua;
	(void)prm;

	switch (ev) {

	case UA_EVENT_CALL_INCOMING:
		if (call_state(call) != CALL_STATE_INCOMING)
			return;

		const char *peer = call_peeruri(call);

		audio_set_devicename(call_audio(call), peer, peer);
		video_set_devicename(call_video(call), peer, peer);

		stream_enable(video_strm(call_video(call)), true);
		stream_enable(audio_strm(call_audio(call)), true);

		(void)call_answer(call, 200, VIDMODE_ON);
		info("auto answer call with %s\n", call_peeruri(call));

		slmix_disp_enable(mix, peer, true);

	default:
		break;
	}
}


int slmix_sip_init(struct mix *mix)
{
	int err;
	char aor[128];

	err = uag_event_register(ua_event_handler, mix);
	if (err)
		return err;

	re_snprintf(aor, sizeof(aor), "<sip:mix@podstock>;regint=0");

	err = ua_alloc(&sip_ua, aor);
	if (err)
		return err;

	err = ua_register(sip_ua);

	return err;
}


int slmix_sip_close(void)
{
	uag_event_unregister(ua_event_handler);

	return 0;
}
