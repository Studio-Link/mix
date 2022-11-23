enum { 
	SESSID_SZ = 32,
	AVATARID_SZ = 32,
	NAME_SZ	= 16
};

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
	char id[SESSID_SZ];
	char avatar_id[AVATARID_SZ];
	char name[NAME_SZ];
};

int slmix_http_listen(struct http_sock **sock, struct mix *mix);
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
int avatar_save(struct session *sess, struct http_conn *conn,
		const struct http_msg *msg);
void http_sreply(struct http_conn *conn, uint16_t scode, const char *reason,
		 const char *ctype, const char *fmt, size_t size,
		 struct session *sess);

/******************************************************************************
 * ws.c
 */
enum ws_type { WS_USERS, WS_CHAT };
int sl_ws_init(void);
int sl_ws_close(void);
int sl_ws_open(struct http_conn *conn, enum ws_type type,
	       const struct http_msg *msg, websock_recv_h *recvh,
	       struct mix *mix);
void sl_ws_send_str(enum ws_type ws_type, char *str);
void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);
void sl_ws_users_auth(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);
