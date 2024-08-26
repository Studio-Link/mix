#include <assert.h>
#include <mix.h>

static struct ua *sip_ua;


static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	struct mix *mix = arg;
	struct le *le;
	int err;

	(void)ua;
	(void)prm;

	switch (ev) {

	case UA_EVENT_CALL_INCOMING:
		if (call_state(call) != CALL_STATE_INCOMING)
			return;

		struct session *sess;
		const char *peer  = call_peeruri(call);
		struct pl peer_pl = PL_INIT;

		pl_set_str(&peer_pl, peer);

		slmix_session_alloc(&sess, mix, NULL, NULL, &peer_pl, false,
				    true);

		audio_set_devicename(call_audio(call), peer, peer);
		video_set_devicename(call_video(call), peer, peer);

		stream_enable(video_strm(call_video(call)), true);
		stream_enable(audio_strm(call_audio(call)), true);

		(void)call_answer(call, 200, VIDMODE_ON);
		info("auto answer call with %s\n", call_peeruri(call));

		amix_mute(peer, false, ++mix->next_speaker_id);

		sess->call	= call;
		sess->connected = true;

		slmix_disp_enable(mix, peer, true);

		LIST_FOREACH(&mix->sessl, le)
		{
			struct session *sesse = le->data;
			struct source_pc *src;

			if (!sesse->user->host)
				continue;

			err = slmix_source_alloc(&src, sesse, peer);
			if (err)
				return;

			err = slmix_source_start(src, mix);
			if (err)
				return;

			src->call = call;
			list_append(&sesse->source_pcl, &src->le, src);
		}
		break;

	case UA_EVENT_CALL_CLOSED:

		le = mix->sessl.head;
		while (le) {
			struct session *sesse = le->data;
			struct le *le2;
			le = le->next;

			le2 = sesse->source_pcl.head;
			while (le2) {
				struct source_pc *src = le2->data;

				le2 = le2->next;

				if (src->call == call) {
					mem_deref(src);
				}
			}

			if (sesse->call == call)
				mem_deref(sesse);
		}

		break;
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
