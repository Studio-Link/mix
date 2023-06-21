#include "mix.h"
#ifdef SLMIX_SD_SOCK
#include "systemd/sd-daemon.h"
#endif


static int handle_put_sdp(struct peer_connection *pc,
			  const struct http_msg *msg)
{
	struct session_description sd = {SDP_NONE, NULL};
	int err			      = 0;

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

	if (sess)
		re_snprintf(sess_head, sizeof(sess_head), "Session-ID: %s\r\n",
			    sess->id);

	err = http_reply(
		conn, scode, reason,
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Referrer-Policy: no-referrer\r\n"

		"Content-Security-Policy: default-src 'self' ws:; "
		"frame-ancestors 'self'; form-action 'self'; img-src * data:;"
		"style-src 'self' 'unsafe-inline';\r\n"
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

#ifndef RELEASE
	/* Default return OPTIONS - needed on dev for preflight CORS Check */
	if (0 == pl_strcasecmp(&msg->met, "OPTIONS")) {
		http_sreply(conn, 204, "OK", "text/html", "", 0, NULL);
		return;
	}
#endif

	/*
	 * Websocket Request
	 */
	if (0 == pl_strcasecmp(&msg->path, "/ws/v1/users")) {
		sl_ws_open(conn, msg, sl_ws_users_auth, mix);
		return;
	}

	/*
	 * API Requests without session
	 */
	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/connect") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

		err = slmix_session_new(mix, &sess, msg);
		if (err == EAUTH) {
			http_sreply(conn, 401, "Unauthorized", "text/html", "",
				    0, NULL);
			return;
		}
		if (err)
			goto err;

		http_sreply(conn, 201, "Created", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/sessions/connected")) {
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

	/*
	 * API Requests with session
	 */

	/* Every requests from here must provide a valid session */
	sess = slmix_session_lookup_hdr(&mix->sessl, msg);
	if (!sess) {
		http_sreply(conn, 404, "Session Not Found", "text/html", "", 0,
			    NULL);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/avatar") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

		avatar_save(sess, conn, msg);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/reauth") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
		err = slmix_session_auth(mix, sess, msg);
		if (err == EAUTH) {
			http_sreply(conn, 401, "Unauthorized", "text/html", "",
				    0, NULL);
			return;
		}
		if (err)
			goto err;

		slmix_session_user_updated(sess);
		slmix_session_save(sess);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/name") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

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

		http_sreply(conn, 204, "Updated", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/sdp/offer") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

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

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/sdp/answer") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		if (msg->clen &&
		    msg_ctype_cmp(&msg->ctyp, "application", "json")) {
			/* @FIXME */
			err = handle_put_sdp(((struct source_pc *)sess
						      ->source_pcl.head->data)
						     ->pc,
					     msg);
			if (err)
				goto err;
		}
		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/record/enable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {
		/* check permission */
		if (!sess->user || !sess->user->host)
			goto err;

		slmix_record(mix, REC_AUDIO_VIDEO);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/record/audio/enable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {
		/* check permission */
		if (!sess->user || !sess->user->host)
			goto err;

		slmix_record(mix, REC_AUDIO);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/record/disable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {
		/* check permission */
		if (!sess->user || !sess->user->host)
			goto err;

		slmix_record(mix, REC_DISABLED);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/speaker") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
		struct pl user_id = PL_INIT;
		struct session *sess_speaker;

		/* check permission */
		if (!sess->user || !sess->user->host)
			goto err;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9]+",
			       &user_id);
		if (err)
			goto err;

		sess_speaker =
			slmix_session_lookup_user_id(&mix->sessl, &user_id);
		if (!sess_speaker)
			goto err;

		err = slmix_session_speaker(sess_speaker, true);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/listener") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {
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

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/hand/enable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		sess->user->hand = true;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/hand/disable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		sess->user->hand = false;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/video/enable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		slmix_session_video(sess, true);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/video/disable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		slmix_session_video(sess, false);

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/audio/enable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		sess->user->audio = true;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/audio/disable") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		sess->user->audio = false;

		err = slmix_session_user_updated(sess);
		if (err)
			goto err;

		http_sreply(conn, 204, "OK", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/hangup") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

		pc_close(sess);

		slmix_session_user_updated(sess);

		http_sreply(conn, 204, "OK", "text/html", "", 0, NULL);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client") &&
	    0 == pl_strcasecmp(&msg->met, "DELETE")) {

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

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/chat") &&
	    0 == pl_strcasecmp(&msg->met, "GET")) {

		char *json;

		err = chat_json(&json, mix);
		if (err)
			goto err;

		http_sreply(conn, 200, "OK", "text/html", json, str_len(json),
			    sess);
		mem_deref(json);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/chat") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

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

err:
	http_sreply(conn, 500, "Error", "text/html", "", 0, sess);
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
	err = sa_set_str(&srv, "127.0.0.1", 9999);
	if (err)
		return err;

	info("listen webui: http://%J\n", &srv);
	err = http_listen(sock, &srv, http_req_handler, mix);

#endif

	return err;
}
