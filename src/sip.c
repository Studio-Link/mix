#include <assert.h>
#include <mix.h>

static struct ua *sip_ua;


static int ws_json(struct session *sess, const struct odict *od)
{
	char *buf = NULL;
	int err;

	if (!sess)
		return EINVAL;

	err = re_sdprintf(&buf, "%H", json_encode_odict, od);
	if (err)
		goto out;

	sl_ws_send_event_self(sess, buf);

out:
	mem_deref(buf);

	return err;
}


static void gather_handler(void *arg)
{
	struct source_pc *src = arg;
	struct mbuf *mb_sdp   = NULL;
	struct odict *od      = NULL;
	enum sdp_type type    = SDP_NONE;
	int err;

	switch (peerconnection_signaling(src->pc)) {

	case SS_STABLE:
		type = SDP_OFFER;
		break;

	case SS_HAVE_LOCAL_OFFER:
		warning("sip gather illegal state\n");
		type = SDP_OFFER;
		break;

	case SS_HAVE_REMOTE_OFFER:
		type = SDP_ANSWER;
		break;
	}

	info("sip: session gathered -- send sdp '%s'\n", sdptype_name(type));

	if (type == SDP_OFFER)
		err = peerconnection_create_offer(src->pc, &mb_sdp);
	else
		err = peerconnection_create_answer(src->pc, &mb_sdp);
	if (err)
		goto out;

	err = session_description_encode(&od, type, mb_sdp);
	if (err) {
		warning("sip: sdp encode error: %m\n", err);
		goto out;
	}

	err = ws_json(src->sess, od);
	if (err) {
		warning("sip: reply ws error: %m\n", err);
		goto out;
	}

	if (type == SDP_ANSWER) {
		err = peerconnection_start_ice(src->pc);
		if (err) {
			warning("sip: failed to start ice (%m)\n", err);
			goto out;
		}
	}

out:
	mem_deref(mb_sdp);
	mem_deref(od);
}


static void estab_handler(struct media_track *media, void *arg)
{
	int err = 0;
	(void)arg;

	info("sip: stream established: '%s'\n",
	     media_kind_name(mediatrack_kind(media)));

	switch (mediatrack_kind(media)) {

	case MEDIA_KIND_AUDIO:
		err = mediatrack_start_audio(media, baresip_ausrcl(),
					     baresip_aufiltl());
		if (err) {
			warning("sip: could not start audio (%m)\n", err);
		}
		break;

	case MEDIA_KIND_VIDEO:
		err = mediatrack_start_video(media);
		if (err) {
			warning("sip: could not start video (%m)\n", err);
		}
		break;

	default:
		break;
	}

	if (err)
		return;

	stream_enable(media_get_stream(media), true);
}


static void close_handler(int err, void *arg)
{

	struct source_pc *src = arg;
	(void)err;

	list_unlink(&src->le);

	mem_deref(src);
}


static void source_dealloc(void *arg)
{
	struct source_pc *src = arg;

	src->pc = mem_deref(src->pc);
}


static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	struct mix *mix = arg;
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

		sess->connected = true;

		slmix_disp_enable(mix, peer, true);

		/* @TODO: move to UA_EVENT_CALL_ESTABLISHED */

		/* Add source preview connections for host users */
		struct le *le;
		struct source_pc *src;
		struct rtc_configuration pc_config = {.offerer = true};
		const struct config *config	   = conf_config();

		LIST_FOREACH(&mix->sessl, le)
		{
			struct session *sesse = le->data;

			if (!sesse->user->host)
				continue;

			warning("sip: add source connection\n");

			src = mem_zalloc(sizeof(struct source_pc),
					 source_dealloc);
			if (!src)
				return;

			src->sess = sesse;

			err = peerconnection_new(&src->pc, &pc_config,
						 mix->mnat, mix->menc,
						 gather_handler, estab_handler,
						 close_handler, src);
			if (err) {
				warning("sip: peerconnection failed (%m)\n",
					err);
				return;
			}

			err = peerconnection_add_audio_track(
				src->pc, config, baresip_aucodecl());
			if (err) {
				warning("sip: add_audio failed (%m)\n", err);
				return;
			}

			err = peerconnection_add_video_track(
				src->pc, config, baresip_vidcodecl());
			if (err) {
				warning("sip: add_video failed (%m)\n", err);
				return;
			}

			list_append(&sesse->source_pcl, &src->le, src);
		}

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
