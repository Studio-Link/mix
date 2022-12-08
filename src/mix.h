#include <re.h>
#include <baresip.h>

enum {
	SESSID_SZ  = 32,
	USERID_SZ  = 32,
	NAME_SZ	   = 32,
	TOKEN_SZ   = 32,
	CHAT_MSGSZ = 1024
};

enum user_event { USER_ADDED, USER_UPDATED, USER_DELETED, CHAT_ADDED };

struct mix {
	struct list sessl;
	struct list chatl;
	const struct mnat *mnat;
	const struct menc *menc;
	struct http_sock *httpsock;
	const char *www_path;
	char token_host[TOKEN_SZ];
	char token_guests[TOKEN_SZ];
	char token_listeners[TOKEN_SZ];
	char token_download[TOKEN_SZ];
	struct rtc_configuration pc_config;
};

struct user {
	char id[USERID_SZ];
	char name[NAME_SZ];
	bool speaker;
	bool host;
	bool video;
	bool audio;
};

struct chat {
	struct le le;
	uint64_t time;
	struct user *user;
	char message[CHAT_MSGSZ];
};

struct session {
	struct le le;
	struct peer_connection *pc;
	struct http_conn *conn_pending;
	char id[SESSID_SZ]; /* Keep secret */
	struct user *user;
	bool connected;
	struct media_track *maudio;
	struct media_track *mvideo;
};

/******************************************************************************
 * avatar.c
 */
int avatar_save(struct session *sess, struct http_conn *conn,
		const struct http_msg *msg);

/******************************************************************************
 * chat.c
 */
int chat_save(struct user *user, struct mix *mix, const struct http_msg *msg);
int chat_json(char **json, struct mix *mix);
int chat_event_json(char **json, enum user_event event, struct chat *chat);

/******************************************************************************
 * http.c
 */
int slmix_http_listen(struct http_sock **sock, struct mix *mix);

/******************************************************************************
 * sess.c
 */
int session_user_updated(struct session *sess);
void session_video(struct session *sess, bool enable);
int session_speaker(struct session *sess, bool enable);
int session_new(struct mix *mix, struct session **sessp,
		const struct http_msg *msg);
int session_start(struct session *sess,
		  const struct rtc_configuration *pc_config,
		  const struct mnat *mnat, const struct menc *menc);
struct session *session_lookup_hdr(const struct list *sessl,
				   const struct http_msg *msg);
struct session *session_lookup(const struct list *sessl,
			       const struct pl *sessid);
struct session *session_lookup_user_id(const struct list *sessl,
				       const struct pl *user_id);
int session_handle_ice_candidate(struct session *sess, const struct odict *od);
void session_close(struct session *sess, int err);
void http_sreply(struct http_conn *conn, uint16_t scode, const char *reason,
		 const char *ctype, const char *fmt, size_t size,
		 struct session *sess);

/******************************************************************************
 * users.c
 */
int users_json(char **json, struct mix *mix);
int user_event_json(char **json, enum user_event event, struct session *sess);


/******************************************************************************
 * ws.c
 */
int sl_ws_init(void);
int sl_ws_close(void);
int sl_ws_open(struct http_conn *conn, const struct http_msg *msg,
	       websock_recv_h *recvh, struct mix *mix);
void sl_ws_send_event(struct session *sess, char *str);
void sl_ws_send_event_all(char *json);
void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);
void sl_ws_users_auth(const struct websock_hdr *hdr, struct mbuf *mb,
		      void *arg);


/******************************************************************************
 * external modules (aumix, vidmix etc.)
 * @TODO: convert to registered functions or shared header
 */
void aumix_mute(char *device, bool mute);
int aumix_record_enable(bool enable);
uint64_t aumix_record_msecs(void);
void vidmix_disp_enable(const char *device, bool enable);
