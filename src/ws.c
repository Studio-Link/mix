#include <re.h>
#include <baresip.h>
#include "mix.h"

static struct websock *ws = NULL;
static struct list wsl;
struct ws_conn {
	struct le le;
	struct websock_conn *c;
	struct mix *mix;
	struct session *sess;
};

enum { KEEPALIVE = 30 * 1000 };

static struct tmr tmr_update;


void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg)
{
	(void)hdr;
	(void)mb;
	(void)arg;
}


void sl_ws_users_auth(const struct websock_hdr *hdr, struct mbuf *mb,
		      void *arg)
{
	struct ws_conn *wsc = arg;
	struct pl sessid;
	char *json = NULL;
	(void)hdr;

	sessid.p = (const char *)mbuf_buf(mb);
	sessid.l = mbuf_get_left(mb);

	wsc->sess = session_lookup(&wsc->mix->sessl, &sessid);
	if (!wsc->sess) {
		websock_close(wsc->c, WEBSOCK_INTERNAL_ERROR,
			      "Session not found");
		mem_deref(wsc);
		return;
	}

	mem_ref(wsc->sess);
	mem_ref(wsc->sess->user);

	wsc->sess->connected = true;

	if (0 == users_json(&json, wsc->mix)) {
		websock_send(wsc->c, WEBSOCK_TEXT, "%s", json);
		json = mem_deref(json);
	}

	if (0 == user_event_json(&json, USER_ADDED, wsc->sess)) {
		sl_ws_send_event(wsc->sess, json);
		json = mem_deref(json);
	}
}


static void conn_destroy(void *arg)
{
	struct ws_conn *ws_conn = arg;
	mem_deref(ws_conn->c);
	if (ws_conn->sess)
		mem_deref(ws_conn->sess->user);
	mem_deref(ws_conn->sess);
	list_unlink(&ws_conn->le);
}


static void close_handler(int err, void *arg)
{
	struct ws_conn *wsc = arg;
	char *json	    = NULL;
	(void)err;

	if (wsc->sess && wsc->sess->user) {
		wsc->sess->connected   = false;
		wsc->sess->user->audio = false;
		wsc->sess->user->video = false;

		if (0 == user_event_json(&json, USER_DELETED, wsc->sess)) {
			sl_ws_send_event(wsc->sess, json);
			json = mem_deref(json);
		}

		vidmix_disp_enable(wsc->sess->id, false);
		wsc->sess->pc = mem_deref(wsc->sess->pc);
	}

	mem_deref(wsc);
}


int sl_ws_open(struct http_conn *conn, const struct http_msg *msg,
	       websock_recv_h *recvh, struct mix *mix)
{
	struct ws_conn *ws_conn;
	int err;

	ws_conn = mem_zalloc(sizeof(*ws_conn), conn_destroy);
	if (!ws_conn)
		return ENOMEM;

	ws_conn->mix = mix;

	err = websock_accept(&ws_conn->c, ws, conn, msg, KEEPALIVE, recvh,
			     close_handler, ws_conn);
	if (err)
		goto out;

	list_append(&wsl, &ws_conn->le, ws_conn);

out:
	if (err)
		mem_deref(ws_conn);

	return err;
}


void sl_ws_send_event(struct session *sess, char *json)
{
	struct le *le;

	if (!json)
		return;

	LIST_FOREACH(&wsl, le)
	{
		struct ws_conn *ws_conn = le->data;
		if (ws_conn->sess == sess)
			continue;
		websock_send(ws_conn->c, WEBSOCK_TEXT, "%s", json);
	}
}


void sl_ws_send_event_all(char *json)
{
	struct le *le;

	if (!json)
		return;

	LIST_FOREACH(&wsl, le)
	{
		struct ws_conn *ws_conn = le->data;
		websock_send(ws_conn->c, WEBSOCK_TEXT, "%s", json);
	}
}


static void update_handler(void *arg)
{
	(void)arg;
	char json[512];
	uint64_t secs = aumix_record_msecs() / 1000;

	if (!secs)
		goto out;

	re_snprintf(json, sizeof(json), "{\"type\": \"rec\", \"t\": %u}",
		    secs);

	sl_ws_send_event_all(json);

out:
	tmr_start(&tmr_update, 1000, update_handler, NULL);
}


int sl_ws_init(void)
{
	int err;

	list_init(&wsl);
	err = websock_alloc(&ws, NULL, NULL);

	tmr_init(&tmr_update);
	tmr_start(&tmr_update, 1000, update_handler, NULL);

	return err;
}


int sl_ws_close(void)
{
	tmr_cancel(&tmr_update);
	list_flush(&wsl);

	if (ws)
		ws = mem_deref(ws);

	return 0;
}
