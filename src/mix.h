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
	char id[32];
	char avatar_id[32];
	char name[16];
};

int slmix_http_listen(struct http_sock **sock, struct mix *mix);
int session_new(struct list *sessl, struct session **sessp);
int session_start(struct session *sess,
		  const struct rtc_configuration *pc_config,
		  const struct mnat *mnat, const struct menc *menc);
struct session *session_lookup(const struct list *sessl,
			       const struct http_msg *msg);
int session_handle_ice_candidate(struct session *sess, const struct odict *od);
void session_close(struct session *sess, int err);
int avatar_save(struct session *sess, struct http_conn *conn, const struct http_msg *msg);
void http_sreply(struct http_conn *conn, uint16_t scode,
			const char *reason, const char *ctype, const char *fmt,
			size_t size, struct session *sess);
