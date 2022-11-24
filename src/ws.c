#include <re.h>
#include <baresip.h>
#include "mix.h"

static struct websock *ws = NULL;
static struct list wsl;
struct ws_conn {
	struct le le;
	struct websock_conn *c;
	enum ws_type type;
	struct mix *mix;
};

enum { KEEPALIVE = 30 * 1000 };


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
	struct session *sess;
	char *json = NULL;
	(void)hdr;

	sessid.p = (const char *)mbuf_buf(mb);
	sessid.l = mbuf_get_left(mb);

	sess = session_lookup(&wsc->mix->sessl, &sessid);
	if (!sess) {
		mem_deref(wsc);
		return;
	}

	if (users_json(&json, wsc->mix) == 0) {
		websock_send(wsc->c, WEBSOCK_TEXT, "%s", json);
		mem_deref(json);
	}
}


static void conn_destroy(void *arg)
{
	struct ws_conn *ws_conn = arg;
	mem_deref(ws_conn->c);
	list_unlink(&ws_conn->le);
}


static void close_handler(int err, void *arg)
{
	struct ws_conn *ws_conn = arg;
	(void)err;

	mem_deref(ws_conn);
}


int sl_ws_open(struct http_conn *conn, enum ws_type type,
	       const struct http_msg *msg, websock_recv_h *recvh,
	       struct mix *mix)
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

	ws_conn->type = type;

	list_append(&wsl, &ws_conn->le, ws_conn);

out:
	if (err)
		mem_deref(ws_conn);

	return err;
}


void sl_ws_send_str(enum ws_type type, char *str)
{
	struct le *le;

	if (!str)
		return;

	LIST_FOREACH(&wsl, le)
	{
		struct ws_conn *ws_conn = le->data;
		if (ws_conn->type != type)
			continue;
		websock_send(ws_conn->c, WEBSOCK_TEXT, "%s", str);
	}
}


int sl_ws_init(void)
{
	int err;

	list_init(&wsl);
	err = websock_alloc(&ws, NULL, NULL);

	return err;
}


int sl_ws_close(void)
{
	list_flush(&wsl);

	if (ws)
		ws = mem_deref(ws);

	return 0;
}
