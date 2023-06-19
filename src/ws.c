#include <re.h>
#include <baresip.h>
#include <mix.h>

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


void sl_ws_session_close(struct session *sess)
{
	struct le *le;

	if (!sess)
		return;

	LIST_FOREACH(&wsl, le)
	{
		struct ws_conn *wsc = le->data;

		if (sess != wsc->sess)
			continue;

		mem_deref(wsc);
		return;
	}
}


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

	wsc->sess = slmix_session_lookup(&wsc->mix->sessl, &sessid);
	if (!wsc->sess) {
		websock_close(wsc->c, WEBSOCK_INVALID_PAYLOAD,
			      "Session not found");
		mem_deref(wsc);
		return;
	}

	mem_ref(wsc->sess);
	mem_ref(wsc->sess->user);

	if (wsc->sess->connected) {
		websock_close(wsc->c, WEBSOCK_INTERNAL_ERROR,
			      "Session in use");
		mem_deref(wsc);
		return;
	}

	wsc->sess->connected = true;

	if (0 == users_json(&json, wsc->mix)) {
		websock_send(wsc->c, WEBSOCK_TEXT, "%s", json);
		json = mem_deref(json);
	}

	bool force = true;
	slmix_update_room();
	slmix_refresh_rooms(&force);

	if (0 == user_event_json(&json, USER_ADDED, wsc->sess)) {
		sl_ws_send_event(wsc->sess, json);
		json = mem_deref(json);
	}
}


static void conn_destroy(void *arg)
{
	struct ws_conn *wsc = arg;
	char *json	    = NULL;

	mem_deref(wsc->c);

	if (wsc->sess && wsc->sess->user) {
		wsc->sess->connected   = false;
		wsc->sess->user->audio = false;
		wsc->sess->user->video = false;

		if (0 == user_event_json(&json, USER_DELETED, wsc->sess)) {
			sl_ws_send_event(wsc->sess, json);
			json = mem_deref(json);
		}

		slmix_disp_enable(wsc->mix, wsc->sess->user->id, false);
		wsc->sess->user->pidx = 0;

		pc_close(wsc->sess);
	}

	if (wsc->sess)
		mem_deref(wsc->sess->user);

	mem_deref(wsc->sess);
	list_unlink(&wsc->le);

	bool force = true;
	slmix_update_room();
	slmix_refresh_rooms(&force);
}


static void close_handler(int err, void *arg)
{
	struct ws_conn *wsc = arg;
	(void)err;

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


static void ws_send_rtcp_stats(struct mix *mix)
{
	struct le *le;
	char json[128];
	const struct rtcp_stats *audio_stat;
	const struct rtcp_stats *video_stat;

	LIST_FOREACH(&mix->sessl, le)
	{

		struct session *sess = le->data;

		if (!sess->connected)
			continue;

		audio_stat = stream_rtcp_stats(media_get_stream(sess->maudio));
		video_stat = stream_rtcp_stats(media_get_stream(sess->mvideo));

		if (!audio_stat || !video_stat)
			continue;

		re_snprintf(json, sizeof(json),
			    "{\"type\": \"stats\","
			    "\"id\": %u, \"stats\": {"
			    "\"artt\": %lu," /* rtt in ms */
			    "\"vrtt\": %lu" /* rtt in ms */
			    "}}",
			    sess->user->speaker_id, audio_stat->rtt / 1000,
			    video_stat->rtt / 1000);
		sl_ws_send_event_all(json);
	}
}


static void update_handler(void *arg)
{
	struct mix *mix = arg;
	char json[128];
	uint64_t secs;

	if (!mix)
		return;

	if (!mix->time_rec_h || !mix->talk_detect_h)
		goto out;

	secs = mix->time_rec_h() / 1000;

	re_snprintf(json, sizeof(json),
		    "{\"type\": \"rec\", \"t\": %lu, \"s\": %u}", secs,
		    mix->talk_detect_h());
	sl_ws_send_event_all(json);

	ws_send_rtcp_stats(mix);

out:
	tmr_start(&tmr_update, 1000, update_handler, mix);
}


int sl_ws_init(void)
{
	int err;

	list_init(&wsl);
	err = websock_alloc(&ws, NULL, NULL);

	tmr_init(&tmr_update);
	tmr_start(&tmr_update, 1000, update_handler, slmix());

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


struct list *sl_ws_list(void)
{
	return &wsl;
}
