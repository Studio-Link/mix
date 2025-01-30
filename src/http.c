#include "mix.h"
#ifdef SLMIX_SD_SOCK
#include "systemd/sd-daemon.h"
#endif

#define ROUTE(route, method)                                                  \
	if (0 == pl_strcasecmp(&msg->path, (route)) &&                        \
	    0 == pl_strcasecmp(&msg->met, (method)))

static int handle_put_sdp(struct peer_connection *pc,
			  const struct http_msg *msg)
{
	struct session_description sd = {SDP_NONE, NULL};
	int err			      = 0;

	if (!pc || !msg)
		return EINVAL;

	info("mix: handle PUT sdp: content is '%r/%r'\n", &msg->ctyp.type,
	     &msg->ctyp.subtype);

	err = session_description_decode(&sd, msg->mb);
	if (err)
		return err;

	err = peerconnection_set_remote_descr(pc, &sd);
	if (err) {
		warning("mix: set remote descr error (%m)\n", err);
		goto out;
	}

	if (sd.type == SDP_ANSWER) {
		err = peerconnection_start_ice(pc);
		if (err) {
			warning("mix: failed to start ice (%m)\n", err);
			goto out;
		}
	}

out:
	session_description_reset(&sd);

	return err;
}


void http_sreply(struct http_conn *conn, uint16_t scode, const char *reason,
		 const char *ctype, const char *fmt, size_t size,
		 struct session *sess)
{
	struct mbuf *mb;
	int err		    = 0;
	char sess_head[128] = {0};
	(void)scode;
	(void)reason;

	mb = mbuf_alloc(size);
	if (!mb) {
		warning("http_sreply: ENOMEM\n");
		return;
	}

	err = mbuf_write_mem(mb, (const uint8_t *)fmt, size);
	if (err) {
		warning("http_sreply: mbuf_write_mem err %m\n", err);
		goto out;
	}

#ifdef RELEASE
	if (sess)
		re_snprintf(sess_head, sizeof(sess_head),
			    "Set-Cookie: mix_session=%s; Path=/; HttpOnly; "
			    "Secure; "
			    "SameSite=Lax; Max-Age=31536000\r\n",
			    sess->id);
#else
	if (sess)
		re_snprintf(sess_head, sizeof(sess_head),
			    "Set-Cookie: mix_session=%s; Path=/; HttpOnly; "
			    "SameSite=Lax; Max-Age=31536000\r\n",
			    sess->id);
#endif

	err = http_reply(
		conn, scode, reason,
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Referrer-Policy: no-referrer\r\n"
#ifndef RELEASE
		/* Only allow wildcard CORS on DEV Builds */
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: *\r\n"
		"Access-Control-Allow-Headers: *\r\n"
		"Access-Control-Expose-Headers: *\r\n"
#endif
		"Status-Reason: %s\r\n"
		"%s"
		"\r\n"
		"%b",
		ctype, mb->end, reason, sess_head, mb->buf, mb->end);
	if (err)
		warning("http_sreply: http_reply err %m\n", err);
out:
	mem_deref(mb);
}


static void http_req_handler(struct http_conn *conn,
			     const struct http_msg *msg, void *arg)
{
	int err		     = 0;
	struct session *sess = NULL;
	struct mix *mix	     = arg;

	if (!conn || !msg)
		return;

	info("conn %r %r %p\n", &msg->met, &msg->path, conn);

#if 0
	/* Header debug */
	struct le *hle;
	LIST_FOREACH(&msg->hdrl, hle)
	{
		struct http_hdr *hdr = hle->data;
		info("  %r: %r\n", &hdr->name, &hdr->val);
	}
#endif

#ifndef RELEASE
	/* Default return OPTIONS - needed on dev for preflight CORS Check */
	if (0 == pl_strcasecmp(&msg->met, "OPTIONS")) {
		http_sreply(conn, 204, "OK", "text/html", "", 0, NULL);
		return;
	}
#endif
	/*
	 * API Requests without session
	 */
	ROUTE("/api/v1/sessions/connected", "GET")
	{
		struct le *le;
		uint32_t cnt = 0;
		char count[ITOA_BUFSZ];
		char *countp;

		LIST_FOREACH(&mix->sessl, le)
		{
			sess = le->data;
			if (sess->connected)
				++cnt;
		}

		countp = str_itoa(cnt, count, 10);
		http_sreply(conn, 200, "OK", "text/html", countp,
			    str_len(countp), NULL);

		return;
	}

	sess = slmix_session_lookup_hdr(&mix->sessl, msg);

	ROUTE("/api/v1/client/connect", "POST")
	{
		if (!sess) {
			err = slmix_session_new(mix, &sess, msg);
			if (err == EAUTH) {
				http_sreply(conn, 401, "Unauthorized",
					    "text/html", "", 0, NULL);
				return;
			}
			if (err)
				goto err;
		}
		else {
			err = slmix_session_auth(mix, sess, msg);
			if (err == EAUTH)
				goto auth;

			if (err)
				goto err;
		}

		if (sess->auth) {
			http_sreply(conn, 201, "Created", "text/html",
				    sess->user->id, str_len(sess->user->id),
				    sess);
		}
		else {
			http_sreply(conn, 201, "Created", "text/html", "", 0,
				    sess);
		}
		return;
	}

	/*
	 * Requests with session
	 */
	/* Every requests from here must provide a valid session */
	if (!sess) {
		http_sreply(conn, 401, "Unauthorized", "text/html", "", 0,
			    NULL);
		return;
	}

	/*
	 * Websocket Request
	 */
	if (0 == pl_strcasecmp(&msg->path, "/ws/v1/users")) {
		sl_ws_open(conn, msg, mix, sess);
		return;
	}

	ROUTE("/api/v1/client/avatar", "POST")
	{
		avatar_save(sess, conn, msg);
		return;
	}

	ROUTE("/api/v1/client/reauth", "POST")
	{
		err = slmix_session_auth(mix, sess, msg);
		if (err == EAUTH)
			goto auth;

		if (err)
			goto err;

		/* generate new session - ensures rooms load new session */
		rand_str(sess->id, sizeof(sess->id));

		slmix_session_user_updated(sess);
		slmix_session_save(sess);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/client/name", "POST")
	{
		struct pl name = PL_INIT;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9 ]+", &name);
		if (err) {
			http_sreply(conn, 400, "Name error", "text/html", "",
				    0, sess);
			return;
		}

		re_snprintf(sess->user->name, sizeof(sess->user->name), "%r",
			    &name);

		if (!str_isset(sess->user->name)) {
			http_sreply(conn, 400, "Name error", "text/html", "",
				    0, sess);
			return;
		}

		slmix_session_save(sess);
		sess->auth = true;

		http_sreply(conn, 204, "Updated", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/sdp/offer", "PUT")
	{
		err = slmix_session_start(sess, &mix->pc_config, mix->mnat,
					  mix->menc);
		if (err)
			goto err;

		if (msg->clen &&
		    msg_ctype_cmp(&msg->ctyp, "application", "json")) {
			err = handle_put_sdp(sess->pc, msg);
			if (err)
				goto err;
		}

		/* async reply */
		mem_deref(sess->conn_pending);
		sess->conn_pending = mem_ref(conn);

		return;
	}

	ROUTE("/api/v1/webrtc/sdp/answer", "PUT")
	{
		struct pl pl_id = PL_INIT;
		struct le *le;

		if (!msg->clen)
			goto err;

		if (!msg_ctype_cmp(&msg->ctyp, "application", "json"))
			goto err;

		err = re_regex(msg->prm.p, msg->prm.l, "id=[0-9]+", &pl_id);
		if (err)
			goto err;

		int32_t src_id = pl_i32(&pl_id);

		LIST_FOREACH(&sess->source_pcl, le)
		{
			struct source_pc *source = le->data;

			if (source->id != src_id)
				continue;

			err = handle_put_sdp(source->pc, msg);
			if (err)
				goto err;

			http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
			return;
		}

		http_sreply(conn, 404, "Not found", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/sdp/candidate", "PUT")
	{
		struct pl pl_id = PL_INIT;
		struct le *le;

		if (!msg->clen)
			goto err;

		if (!msg_ctype_cmp(&msg->ctyp, "application", "json"))
			goto err;

		err = re_regex(msg->prm.p, msg->prm.l, "id=[0-9]+", &pl_id);
		if (err)
			goto err;

		enum { HASH_SIZE = 4, MAX_DEPTH = 2 };
		struct odict *od = NULL;

		err = json_decode_odict(&od, HASH_SIZE,
					(char *)mbuf_buf(msg->mb),
					mbuf_get_left(msg->mb), MAX_DEPTH);
		if (err) {
			warning("http: candidate could not decode json (%m)\n",
				err);
			goto err;
		}

		int32_t src_id = pl_i32(&pl_id);

		LIST_FOREACH(&sess->source_pcl, le)
		{
			struct source_pc *source = le->data;

			if (source->id != src_id)
				continue;

			err = slmix_handle_ice_candidate(source->pc, od);
			if (err)
				goto err;

			mem_deref(od);

			http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
			return;
		}

		mem_deref(od);

		http_sreply(conn, 404, "Not found", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/record/enable", "PUT")
	{
		/* check permission */
		if (!sess->user || !sess->user->host)
			goto auth;

		slmix_record(mix, REC_AUDIO_VIDEO);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/record/audio/enable", "PUT")
	{
		/* check permission */
		if (!sess->user || !sess->user->host)
			goto auth;

		slmix_record(mix, REC_AUDIO);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/record/disable", "PUT")
	{
		/* check permission */
		if (!sess->user || !sess->user->host)
			goto auth;

		slmix_record(mix, REC_DISABLED);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/client/speaker", "POST")
	{
		struct pl user_id = PL_INIT;
		struct session *sess_speaker;

		/* check host permission if show */
		if (!sess->user || (!sess->user->host && sess->mix->show))
			goto auth;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9]+",
			       &user_id);
		if (err)
			goto err;

		sess_speaker =
			slmix_session_lookup_user_id(&mix->sessl, &user_id);
		if (!sess_speaker)
			goto err;

		/* if not a show, allow only user itself */
		if (!sess->mix->show && sess != sess_speaker)
			goto auth;

		err = slmix_session_speaker(sess_speaker, true);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/client/listener", "POST")
	{
		struct pl user_id = PL_INIT;
		struct session *sess_listener;

		if (!sess->user)
			goto err;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9]+",
			       &user_id);
		if (err)
			goto err;

		sess_listener =
			slmix_session_lookup_user_id(&mix->sessl, &user_id);
		if (!sess_listener)
			goto err;

		/* check permission, allow self listener downgrade */
		if (sess->user->host || sess == sess_listener) {
			err = slmix_session_speaker(sess_listener, false);
			if (err)
				goto err;
		}

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/hand/enable", "PUT")
	{
		sess->user->hand = true;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/hand/disable", "PUT")
	{
		sess->user->hand = false;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/video/enable", "PUT")
	{

		slmix_session_video(sess, true);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/video/disable", "PUT")
	{
		slmix_session_video(sess, false);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/audio/enable", "PUT")
	{
		sess->user->audio = true;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/audio/disable", "PUT")
	{
		sess->user->audio = false;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/focus", "PUT")
	{
		char user[512]	  = {0};
		struct pl user_id = PL_INIT;

		/* check permission */
		if (!sess->user->host)
			goto err;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9@:]+",
			       &user_id);
		if (err)
			goto err;

		pl_strcpy(&user_id, user, sizeof(user));

		vmix_disp_focus(user);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/solo/enable", "PUT")
	{
		char user[512]	  = {0};
		struct pl user_id = PL_INIT;

		/* check permission */
		if (!sess->user->host)
			goto err;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9@:]+",
			       &user_id);
		if (err)
			goto err;

		pl_strcpy(&user_id, user, sizeof(user));

		struct session *sess_speaker =
			slmix_session_lookup_user_id(&mix->sessl, &user_id);
		if (!sess_speaker)
			goto err;

		slmix_session_video_solo(sess_speaker->user, true);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/solo/disable", "PUT")
	{
		/* check permission */
		if (!sess->user->host)
			goto err;

		slmix_session_video_solo(NULL, false);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/disp/enable", "PUT")
	{
		char user[512] = {0};

		/* check permission */
		if (!sess->user->host)
			goto err;

		mbuf_read_str(msg->mb, user, sizeof(user));

		mix->disp_enable_h(user, true);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/disp/disable", "PUT")
	{
		char user[512] = {0};

		/* check permission */
		if (!sess->user->host)
			goto err;

		mbuf_read_str(msg->mb, user, sizeof(user));

		mix->disp_enable_h(user, false);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/amix/enable", "PUT")
	{
		char user[512]	  = {0};
		struct pl user_id = PL_INIT;

		/* check permission */
		if (!sess->user->host)
			goto err;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9@:]+",
			       &user_id);
		if (err)
			goto err;

		pl_strcpy(&user_id, user, sizeof(user));

		struct session *sess_listener =
			slmix_session_lookup_user_id(&mix->sessl, &user_id);
		if (!sess_listener)
			goto err;

		sess_listener->user->audio = true;
		amix_mute(user, false, 0);

		slmix_session_user_updated(sess_listener);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/amix/disable", "PUT")
	{
		char user[512]	  = {0};
		struct pl user_id = PL_INIT;

		/* check permission */
		if (!sess->user->host)
			goto err;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9@:]+",
			       &user_id);
		if (err)
			goto err;

		pl_strcpy(&user_id, user, sizeof(user));

		pl_set_str(&user_id, user);
		struct session *sess_listener =
			slmix_session_lookup_user_id(&mix->sessl, &user_id);
		if (!sess_listener)
			goto err;

		sess_listener->user->audio = false;
		amix_mute(user, true, 0);

		slmix_session_user_updated(sess_listener);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/webrtc/stats", "POST")
	{
		struct sl_httpconn *http_conn;
		char metric_url[URL_SZ] = {0};

		err = sl_httpc_alloc(&http_conn, NULL, NULL, NULL);
		if (err)
			goto err;

		re_snprintf(metric_url, sizeof(metric_url),
			    METRICS_URL "/instance/%s/user/%s", mix->room,
			    sess->user->id);

		struct mbuf *body = mbuf_alloc(mbuf_get_left(msg->mb));
		mbuf_write_mem(body, mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb));

		struct pl header_gzip;
		pl_set_str(&header_gzip, "Content-Encoding: gzip");
		http_reqconn_add_header(http_conn->conn, &header_gzip);

		err = sl_httpc_req(http_conn, SL_HTTP_POST, metric_url, body);
		mem_deref(http_conn);
		mem_deref(body);

		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	ROUTE("/api/v1/client/hangup", "POST")
	{
		pc_close(sess);

		slmix_session_user_updated(sess);

		http_sreply(conn, 204, "OK", "text/html", "", 0, NULL);
		return;
	}

	ROUTE("/api/v1/client", "DELETE")
	{
		struct pl sess_id;
		/* draft-ietf-wish-whip-03 */
		info("mix: DELETE -> disconnect\n");

		pl_set_str(&sess_id, sess->id);
		slmix_db_del(slmix_db_sess(), &sess_id);

		avatar_delete(sess);

		info("mix: closing session %s\n", sess->id);
		slmix_session_close(sess, 0);
		sess = NULL;

		http_sreply(conn, 204, "OK", "text/html", "", 0, NULL);
		return;
	}

	ROUTE("/api/v1/emoji", "PUT")
	{
		struct pl id;
		char json[128];
		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[0-9]+", &id);
		if (err)
			goto err;

		re_snprintf(json, sizeof(json),
			    "{\"type\": \"emoji\", \"id\": %r }", &id);

		sl_ws_send_event_all(json);
		http_sreply(conn, 204, "OK", "text/html", "", 0, NULL);
		return;
	}

	ROUTE("/api/v1/chat", "GET")
	{
		char *json;

		err = chat_json(&json, mix);
		if (err)
			goto err;

		http_sreply(conn, 200, "OK", "text/html", json, str_len(json),
			    sess);
		mem_deref(json);
		return;
	}

	ROUTE("/api/v1/chat", "POST")
	{
		err = chat_save(sess->user, mix, msg);
		if (err && err != ENODATA)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	/*
	 * Default 404
	 */
	http_sreply(conn, 404, "Not found", "text/html", "", 0, sess);
	return;

auth:
	http_sreply(conn, 403, "Forbidden", "text/html", "", 0, sess);
	return;

err:
	http_sreply(conn, 500, "Error", "text/html", "", 0, sess);
	return;
}


int slmix_http_listen(struct http_sock **sock, struct mix *mix)
{
	int err;

	if (!sock)
		return EINVAL;

#ifdef SLMIX_UNIX_SOCK
	struct sa srv;
	re_sock_t fd;
	err = sa_set_str(&srv, "unix:/run/slmix.sock", 0);
	if (err)
		return err;

	info("listen webui: http://%j\n", &srv);

	err = unixsock_listen_fd(&fd, &srv);
	if (err)
		return err;

	err = http_listen_fd(sock, fd, http_req_handler, mix);
#elif defined(SLMIX_SD_SOCK)
	re_sock_t fd;

	if (sd_listen_fds(0) != 1) {
		info("http_listen: No or to many systemd fds\n");
		return EMFILE;
	}
	fd  = SD_LISTEN_FDS_START + 0;
	err = http_listen_fd(sock, fd, http_req_handler, mix);
#else
	struct sa srv;
	err = sa_set_str(&srv, slmix_config_listen(), 9999);
	if (err)
		return err;

	info("listen webui: http://%J\n", &srv);
	err = http_listen(sock, &srv, http_req_handler, mix);

#endif

	return err;
}
