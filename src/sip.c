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

	err = odict_entry_add(od, "id", ODICT_INT, src->id);
	if (err) {
		warning("sip: odict id error: %m\n", err);
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
	int err		      = 0;
	struct source_pc *src = arg;

	info("sip: webrtc stream established: '%s'\n",
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
		video_set_devicename(media_get_video(media), src->dev,
				     "dummy");
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

	mem_deref(src);
}


static void source_dealloc(void *arg)
{
	struct source_pc *src = arg;
	struct odict *od;

	list_unlink(&src->le);

	int err = odict_alloc(&od, 1);
	if (!err) {
		odict_entry_add(od, "type", ODICT_STRING, "source_close");
		odict_entry_add(od, "id", ODICT_INT, src->id);
		ws_json(src->sess, od);
		mem_deref(od);
	}

	src->pc = mem_deref(src->pc);
}


static int32_t source_id_next(struct source_pc *src)
{
	if (!src || !src->sess)
		return -1;

	if (!src->sess->source_pcl.tail)
		return 0;

	struct source_pc *last = src->sess->source_pcl.tail->data;

	if (!last)
		return -1;

	return last->id + 1;
}


static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	struct mix *mix	      = arg;
	struct config *config = conf_config();
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

		sess->call	= call;
		sess->connected = true;

		slmix_disp_enable(mix, peer, true);

		/* @TODO: move to UA_EVENT_CALL_ESTABLISHED */

		/* Add source preview connections for host users */
		struct rtc_configuration pc_config = {.offerer = true};

		re_snprintf(config->video.src_mod,
			    sizeof(config->video.src_mod), "vmix_pktsrc");

		LIST_FOREACH(&mix->sessl, le)
		{
			struct session *sesse = le->data;

			if (!sesse->user->host)
				continue;

			warning("sip: add source connection\n");


			struct source_pc *src = mem_zalloc(
				sizeof(struct source_pc), source_dealloc);
			if (!src)
				return;

			src->call = call;
			src->sess = sesse;

			re_snprintf(src->dev, sizeof(src->dev), "pktsrc%s",
				    peer);

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
				src->pc, config, baresip_aucodecl(),
				SDP_SENDONLY);
			if (err) {
				warning("sip: add_audio failed (%m)\n", err);
				return;
			}

			err = peerconnection_add_video_track(
				src->pc, config, baresip_vidcodecl(),
				SDP_SENDONLY);
			if (err) {
				warning("sip: add_video failed (%m)\n", err);
				return;
			}

			src->id = source_id_next(src);
			if (src->id == -1)
				warning("sip: set source id failed!\n");

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

	/* restore default vmix source module config */
	re_snprintf(config->video.src_mod, sizeof(config->video.src_mod),
		    "vmix");
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
