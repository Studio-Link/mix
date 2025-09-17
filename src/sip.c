#include <mix.h>

static struct ua *sip_ua;
static struct list sip_sessl = LIST_INIT;

struct sip_sess_e {
	struct le le;
	struct call *call;
	struct session *sess;
};


static void destruct_sess_e(void *arg)
{
	struct sip_sess_e *e = arg;

	char *json = NULL;
	user_event_json(&json, USER_DELETED, e->sess);
	sl_ws_send_event(e->sess, json);
	mem_deref(json);

	list_unlink(&e->le);
	mem_deref(e->sess);
}


static void ua_event_handler(enum bevent_ev ev, struct bevent *event,
			     void *arg)
{
	struct mix *mix		  = arg;
	struct call *call	  = bevent_get_call(event);
	const struct sip_msg *msg = bevent_get_msg(event);
	struct sip_sess_e *e	  = NULL;
	int err			  = 0;

	switch (ev) {

	case BEVENT_SIPSESS_CONN:
		struct ua *ua = uag_find_msg(msg);
		ua_accept(ua, msg);

		break;
	case BEVENT_CALL_INCOMING:
		if (call_state(call) != CALL_STATE_INCOMING)
			return;

		struct session *sess;
		const char *peer  = call_peeruri(call);
		struct pl peer_pl = PL_INIT;

		pl_set_str(&peer_pl, peer);

		err = slmix_session_alloc(&sess, mix, NULL, NULL, &peer_pl,
					  false, true);
		if (err)
			goto hangup;

		e = mem_zalloc(sizeof(struct sip_sess_e), destruct_sess_e);
		if (!e) {
			mem_deref(sess);
			goto hangup;
		}

		e->call = call;
		e->sess = sess;

		list_append(&sip_sessl, &e->le, e);

		pl_strcpy(&peer_pl, sess->user->id, sizeof(sess->user->id));

		audio_set_devicename(call_audio(call), peer, peer);
		video_set_devicename(call_video(call), peer, peer);

		stream_enable(video_strm(call_video(call)), true);
		stream_enable(audio_strm(call_audio(call)), true);

		(void)call_answer(call, 200, VIDMODE_ON);
		info("auto answer call with %s\n", call_peeruri(call));

		amix_mute(peer, false, ++mix->next_speaker_id);

		sess->call	  = call;
		sess->connected	  = true;
		sess->user->video = true;

		sess->user->pidx = slmix_disp_enable(mix, peer, true);


		slmix_source_append_all(mix, call, peer);

		break;

	case BEVENT_CALL_CLOSED: {
		slmix_source_deref(mix, call, NULL);
		struct le *le;
		struct le *le_tmp;
		LIST_FOREACH_SAFE(&sip_sessl, le, le_tmp)
		{
			e = le->data;

			if (e->call != call)
				continue;

			mem_deref(e);
		}

		break;
	}
	default:
		break;
	}

	return;
hangup:
	call_hangup(call, 500, "Server Error");
}


int slmix_sip_init(struct mix *mix)
{
	int err;
	char aor[128];

	err = bevent_register(ua_event_handler, mix);
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
	bevent_unregister(ua_event_handler);

	return 0;
}
