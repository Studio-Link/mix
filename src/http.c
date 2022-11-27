#include <re.h>
#include <baresip.h>
#include "mix.h"
#ifdef SLMIX_SD_SOCK
#include "systemd/sd-daemon.h"
#endif


static int handle_put_sdp(struct session *sess, const struct http_msg *msg)
{
	struct session_description sd = {SDP_NONE, NULL};
	int err			      = 0;

	info("mix: handle PUT sdp: content is '%r/%r'\n", &msg->ctyp.type,
	     &msg->ctyp.subtype);

	err = session_description_decode(&sd, msg->mb);
	if (err)
		return err;

	err = peerconnection_set_remote_descr(sess->pc, &sd);
	if (err) {
		warning("mix: set remote descr error (%m)\n", err);
		goto out;
	}

	if (sd.type == SDP_ANSWER) {
		err = peerconnection_start_ice(sess->pc);
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
		"%s"
		"\r\n"
		"%b",
		ctype, mb->end, sess_head, mb->buf, mb->end);
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

	/*
	 * Websocket Request
	 */
	if (0 == pl_strcasecmp(&msg->path, "/ws/v1/users")) {
		sl_ws_open(conn, msg, sl_ws_users_auth, mix);
		return;
	}


#ifndef RELEASE
	/* Default return OPTIONS - needed on dev for preflight CORS Check */
	if (0 == pl_strcasecmp(&msg->met, "OPTIONS")) {
		http_sreply(conn, 200, "OK", "text/html", "", 0, NULL);
		return;
	}
#endif

	if (0 != pl_strcasecmp(&msg->path, "/api/v1/client/connect")) {
		sess = session_lookup_hdr(&mix->sessl, msg);
		if (!sess) {
			http_sreply(conn, 404, "Session Not Found",
				    "text/html", "", 0, NULL);
			return;
		}
	}

	/*
	 * API Requests
	 */

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/connect") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

		err = session_new(&mix->sessl, &sess);
		if (err)
			goto err;

		http_sreply(conn, 201, "Created", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/avatar") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

		avatar_save(sess, conn, msg);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client/name") &&
	    0 == pl_strcasecmp(&msg->met, "POST")) {

		struct pl name = PL_INIT;

		err = re_regex((char *)mbuf_buf(msg->mb),
			       mbuf_get_left(msg->mb), "[a-zA-Z0-9]+", &name);
		if (err)
			goto err;

		re_snprintf(sess->user->name, sizeof(sess->user->name), "%r",
			    &name);

		if (!str_isset(sess->user->name)) {
			http_ereply(conn, 400, "Invalid name");
		}

		http_sreply(conn, 201, "Updated", "text/html", "", 0, sess);
		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/webrtc/sdp") &&
	    0 == pl_strcasecmp(&msg->met, "PUT")) {

		err = session_start(sess, &mix->pc_config, mix->mnat,
				    mix->menc);
		if (err)
			goto err;

		if (msg->clen &&
		    msg_ctype_cmp(&msg->ctyp, "application", "json")) {
			warning("handle put\n");
			err = handle_put_sdp(sess, msg);
			if (err)
				goto err;
		}

		/* async reply */
		mem_deref(sess->conn_pending);
		sess->conn_pending = mem_ref(conn);

		return;
	}

	if (0 == pl_strcasecmp(&msg->path, "/api/v1/client") &&
	    0 == pl_strcasecmp(&msg->met, "DELETE")) {

		/* draft-ietf-wish-whip-03 */
		info("mix: DELETE -> disconnect\n");

		sess = session_lookup_hdr(&mix->sessl, msg);
		if (!sess) {
			http_ereply(conn, 404, "Session Not Found");
			return;
		}

		info("mix: closing session %s\n", sess->id);
		session_close(sess, 0);
		sess = NULL;

		http_sreply(conn, 200, "OK", "text/html", "", 0, sess);
	}

	/* Default 404 return */
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

	re_printf("listen webui: http://%j\n", &srv);

	err = unixsock_listen_fd(&fd, &srv);
	if (err)
		return err;

	err = http_listen_fd(sock, fd, http_req_handler, mix);
#elif defined(SLMIX_SD_SOCK)
	re_sock_t fd;

	if (sd_listen_fds(0) != 1) {
		re_printf("http_listen: No or to many systemd fds\n");
		return EMFILE;
	}
	fd  = SD_LISTEN_FDS_START + 0;
	err = http_listen_fd(sock, fd, http_req_handler, mix);
#else
	struct sa srv;
	err = sa_set_str(&srv, "127.0.0.1", 9999);
	if (err)
		return err;

	re_printf("listen webui: http://%J\n", &srv);
	err = http_listen(sock, &srv, http_req_handler, mix);

#endif

	return err;
}
