/**
 * @file sess.c  Studio Link MIX -- session
 *
 * Copyright (C) 2010 - 2022 Alfred E. Heggestad
 * Copyright (C) 2022 Sebastian Reimers
 */

#include <string.h>
#include <mix.h>


static void destructor(void *data)
{
	struct session *sess = data;

	list_unlink(&sess->le);
	sess->conn_pending = mem_deref(sess->conn_pending);
	sess->pc	   = mem_deref(sess->pc);
	sess->user	   = mem_deref(sess->user);

	list_flush(&sess->source_pcl);
}


void pc_close(struct session *sess)
{
	sess->pc     = mem_deref(sess->pc);
	sess->maudio = NULL;
	sess->mvideo = NULL;

	if (sess->user) {
		sess->user->pidx  = 0;
		sess->user->audio = false;
		sess->user->video = false;
		sess->user->hand  = false;
	}
}


static void pc_gather_handler(void *arg)
{
	struct session *sess = arg;
	struct mbuf *mb_sdp  = NULL;
	struct odict *od     = NULL;
	enum sdp_type type   = SDP_NONE;
	int err;

	switch (peerconnection_signaling(sess->pc)) {

	case SS_STABLE:
		type = SDP_OFFER;
		break;

	case SS_HAVE_LOCAL_OFFER:
		warning("illegal state\n");
		type = SDP_OFFER;
		break;

	case SS_HAVE_REMOTE_OFFER:
		type = SDP_ANSWER;
		break;
	}

	info("mix: session gathered -- send sdp '%s'\n", sdptype_name(type));

	if (type == SDP_OFFER)
		err = peerconnection_create_offer(sess->pc, &mb_sdp);
	else
		err = peerconnection_create_answer(sess->pc, &mb_sdp);
	if (err)
		goto out;

	err = session_description_encode(&od, type, mb_sdp);
	if (err)
		goto out;

	err = http_reply_json(sess->conn_pending, sess->id, od);
	if (err) {
		warning("mix: reply error: %m\n", err);
		goto out;
	}

	if (type == SDP_ANSWER) {

		err = peerconnection_start_ice(sess->pc);
		if (err) {
			warning("mix: failed to start ice (%m)\n", err);
			goto out;
		}
	}

out:
	mem_deref(mb_sdp);
	mem_deref(od);

	if (err)
		slmix_session_close(sess, err);
}


static void pc_estab_handler(struct media_track *media, void *arg)
{
	struct session *sess = arg;
	int err		     = 0;

	if (!sess->user)
		return;

	info("mix: stream established: '%s'\n",
	     media_kind_name(mediatrack_kind(media)));

	switch (mediatrack_kind(media)) {

	case MEDIA_KIND_AUDIO:
		audio_set_devicename(media_get_audio(media), sess->user->id,
				     sess->user->id);
		err = mediatrack_start_audio(media, baresip_ausrcl(),
					     baresip_aufiltl());
		if (err) {
			warning("mix: could not start audio (%m)\n", err);
		}
		sess->maudio = media;
		/* Currently audio is muted only, never disabled */
		stream_enable(media_get_stream(media), true);
		break;

	case MEDIA_KIND_VIDEO:
		video_set_devicename(media_get_video(media), sess->user->id,
				     sess->user->id);
		err = mediatrack_start_video(media);
		if (err) {
			warning("mix: could not start video (%m)\n", err);
		}
		sess->mvideo = media;
		stream_enable(media_get_stream(media), false);
		stream_enable_tx(media_get_stream(media), true);
		break;

	default:
		break;
	}

	if (err) {
		slmix_session_close(sess, err);
		return;
	}

	slmix_session_speaker(sess, sess->user->speaker);
}


static void pc_close_handler(int err, void *arg)
{
	struct session *sess = arg;

	warning("mix: session closed (%m)\n", err);

	slmix_session_user_updated(sess);
}


int slmix_session_start(struct session *sess,
			const struct rtc_configuration *pc_config,
			const struct mnat *mnat, const struct menc *menc)
{
	const struct config *config = conf_config();
	int err;

	if (!sess)
		return EINVAL;

	/* Close old connection */
	pc_close(sess);

	err = peerconnection_new(&sess->pc, pc_config, mnat, menc,
				 pc_gather_handler, pc_estab_handler,
				 pc_close_handler, sess);
	if (err) {
		warning("mix: session alloc failed (%m)\n", err);
		return err;
	}

	err = peerconnection_add_audio_track(sess->pc, config,
					     baresip_aucodecl());
	if (err) {
		warning("mix: add_audio failed (%m)\n", err);
		return err;
	}

	err = peerconnection_add_video_track(sess->pc, config,
					     baresip_vidcodecl());
	if (err) {
		warning("mix: add_video failed (%m)\n", err);
		return err;
	}

	return 0;
}


int slmix_session_alloc(struct session **sessp, struct mix *mix,
			const struct pl *sess_id, const struct pl *user_id,
			const struct pl *name, bool host, bool speaker)
{
	struct session *sess;
	struct user *user;

	sess = mem_zalloc(sizeof(*sess), destructor);
	if (!sess)
		return ENOMEM;

	user = mem_zalloc(sizeof(*user), NULL);
	if (!user) {
		mem_deref(sess);
		return ENOMEM;
	}

	if (sess_id && user_id) {
		info("session: create from database\n");
		pl_strcpy(sess_id, sess->id, sizeof(sess->id));
		pl_strcpy(user_id, user->id, sizeof(user->id));
	}
	else {
		/* generate a unique session and user id */
		info("session: create new\n");
		rand_str(sess->id, sizeof(sess->id));
		rand_str(user->id, sizeof(user->id));
	}

	if (name) {
		pl_strcpy(name, user->name, sizeof(user->name));
	}

	user->host = host;

	if (mix->show)
		user->speaker = speaker;
	else
		user->speaker = true;

	sess->user = user;
	sess->mix  = mix;

	list_append(&mix->sessl, &sess->le, sess);

	*sessp = sess;

	return 0;
}


int slmix_session_auth(struct mix *mix, struct session *sess,
		       const struct http_msg *msg)
{
	struct pl token = PL_INIT;

	re_regex((char *)mbuf_buf(msg->mb), mbuf_get_left(msg->mb),
		 "[a-zA-Z0-9]+", &token);

	if (token.l > 0) {
		if (str_isset(mix->token_host) &&
		    0 == pl_strcmp(&token, mix->token_host)) {

			info("sess: host token\n");
			sess->user->host    = true;
			sess->user->speaker = true;
		}
		else if (str_isset(mix->token_guests) &&
			 0 == pl_strcmp(&token, mix->token_guests)) {

			info("sess: guest token\n");
			sess->user->host    = false;
			sess->user->speaker = true;
		}
		else if (str_isset(mix->token_listeners) &&
			 0 == pl_strcmp(&token, mix->token_listeners)) {

			info("sess: listener token\n");
			sess->user->host    = false;
			sess->user->speaker = false;
		}
		else {
			return EAUTH;
		}
	}

	return 0;
}


int slmix_session_new(struct mix *mix, struct session **sessp,
		      const struct http_msg *msg)
{
	int err;

	err = slmix_session_alloc(sessp, mix, NULL, NULL, NULL, false, false);
	if (err)
		return err;

	return slmix_session_auth(mix, *sessp, msg);
}


struct session *slmix_session_lookup_hdr(const struct list *sessl,
					 const struct http_msg *msg)
{
	const struct http_hdr *hdr;

	hdr = http_msg_xhdr(msg, "Session-ID");
	if (!hdr) {
		warning("mix: no Session-ID header\n");
		return NULL;
	}

	return slmix_session_lookup(sessl, &hdr->val);
}


struct session *slmix_session_lookup(const struct list *sessl,
				     const struct pl *sessid)
{
	struct mbuf mb;
	struct pl pl_user_id = PL_INIT;
	struct pl pl_name    = PL_INIT;
	struct pl pl_host    = PL_INIT;
	struct pl pl_speaker = PL_INIT;
	bool host, speaker;
	struct session *sess;
	int err;

	for (struct le *le = sessl->head; le; le = le->next) {
		sess = le->data;

		if (0 == pl_strcasecmp(sessid, sess->id))
			return sess;
	}

	/* Session DB lookup fallback */
	mbuf_init(&mb);
	err = slmix_db_get(slmix_db_sess(), sessid, &mb);
	if (err)
		goto out;

	err = re_regex((const char *)mb.buf, mb.end - 1,
		       "[^;]+;[^;]+;[^;]+;[^;]+;[^;]+", NULL, &pl_user_id,
		       &pl_name, &pl_host, &pl_speaker);
	if (err)
		goto out;

	pl_bool(&host, &pl_host);
	pl_bool(&speaker, &pl_speaker);

	err = slmix_session_alloc(&sess, slmix(), sessid, &pl_user_id,
				  &pl_name, host, speaker);
	if (!err) {
		mbuf_reset(&mb);
		return sess;
	}

out:
	mbuf_reset(&mb);
	warning("mix: session not found (%r)\n", sessid);

	return NULL;
}


struct session *slmix_session_lookup_user_id(const struct list *sessl,
					     const struct pl *user_id)
{

	for (struct le *le = sessl->head; le; le = le->next) {

		struct session *sess = le->data;

		if (0 == pl_strcmp(user_id, sess->user->id))
			return sess;
	}

	warning("mix: session user_id not found (%r)\n", user_id);

	return NULL;
}


int slmix_session_handle_ice_candidate(struct session *sess,
				       const struct odict *od)
{
	const char *cand, *mid;
	struct pl pl_cand;
	char *cand2 = NULL;
	int err;

	cand = odict_string(od, "candidate");
	mid  = odict_string(od, "sdpMid");
	if (!cand || !mid) {
		warning("mix: candidate: missing 'candidate' or "
			"'mid'\n");
		return EPROTO;
	}

	err = re_regex(cand, str_len(cand), "candidate:[^]+", &pl_cand);
	if (err)
		return err;

	pl_strdup(&cand2, &pl_cand);

	peerconnection_add_ice_candidate(sess->pc, cand2, mid);

	mem_deref(cand2);

	return 0;
}


void slmix_session_close(struct session *sess, int err)
{
	if (err)
		warning("mix: session '%s' closed (%m)\n", sess->id, err);
	else
		info("mix: session '%s' closed\n", sess->id);

	pc_close(sess);
	sl_ws_session_close(sess);

	if (err) {
		http_ereply(sess->conn_pending, 500, "Session closed");
	}

	mem_deref(sess);
}


int slmix_session_user_updated(struct session *sess)
{
	char *json = NULL;
	int err;

	if (!sess)
		return EINVAL;

	err = user_event_json(&json, USER_UPDATED, sess);
	if (err)
		return err;

	sl_ws_send_event_all(json);
	json = mem_deref(json);

	return 0;
}


void slmix_session_video(struct session *sess, bool enable)
{
	if (!sess || !sess->user)
		return;

	if (!sess->user->speaker)
		return;

	sess->user->video = enable;

	sess->user->pidx =
		slmix_disp_enable(sess->mix, sess->user->id, enable);

	if (enable)
		stream_flush(media_get_stream(sess->mvideo));

	slmix_session_user_updated(sess);
}


int slmix_session_speaker(struct session *sess, bool enable)
{
	if (!sess || !sess->mix || !sess->user)
		return EINVAL;

	if (enable && !sess->user->speaker_id)
		sess->user->speaker_id = ++sess->mix->next_speaker_id;

	sess->user->speaker = enable;
	amix_mute(sess->user->id, !enable, sess->user->speaker_id);
	stream_enable(media_get_stream(sess->mvideo), enable);
	stream_enable_tx(media_get_stream(sess->mvideo), true);
	sess->user->hand = false;

	/* only allow disable for privacy reasons */
	if (!enable) {
		sess->user->video = false;
		sess->user->pidx =
			slmix_disp_enable(sess->mix, sess->user->id, false);
	}

	return slmix_session_user_updated(sess);
}


int slmix_session_save(struct session *sess)
{
	char str[128] = {0};
	struct pl key;

	if (!sess || !sess->user)
		return EINVAL;

	re_snprintf(str, sizeof(str), "%llu;%s;%s;%d;%d",
		    tmr_jiffies_rt_usec() / 1000, sess->user->id,
		    sess->user->name, sess->user->host, sess->user->speaker);

	pl_set_str(&key, sess->id);

	return slmix_db_put(slmix_db_sess(), &key, str, str_len(str) + 1);
}
