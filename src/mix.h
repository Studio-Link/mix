#include <re.h>
#include <baresip.h>

enum { SESSID_SZ = 32, USERID_SZ = 32, NAME_SZ = 16 };

enum user_event { USER_ADDED, USER_UPDATED, USER_DELETED };

struct mix {
	struct list sessl;
	const struct mnat *mnat;
	const struct menc *menc;
	struct http_sock *httpsock;
	const char *www_path;
	struct rtc_configuration pc_config;
};

struct session {
	struct le le;
	struct peer_connection *pc;
	struct http_conn *conn_pending;
	char id[SESSID_SZ]; /* Keep secret */
	char user_id[USERID_SZ];
	char name[NAME_SZ];
	bool connected;
};

/******************************************************************************
 * avatar.c
 */
int avatar_save(struct session *sess, struct http_conn *conn,
		const struct http_msg *msg);

/******************************************************************************
 * http.c
 */
int slmix_http_listen(struct http_sock **sock, struct mix *mix);

/******************************************************************************
 * sess.c
 */
int session_new(struct list *sessl, struct session **sessp);
int session_start(struct session *sess,
		  const struct rtc_configuration *pc_config,
		  const struct mnat *mnat, const struct menc *menc);
struct session *session_lookup_hdr(const struct list *sessl,
				   const struct http_msg *msg);
struct session *session_lookup(const struct list *sessl,
			       const struct pl *sessid);
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
void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);
void sl_ws_users_auth(const struct websock_hdr *hdr, struct mbuf *mb,
		      void *arg);
