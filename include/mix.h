#include <re.h>
#include <baresip.h>

enum {
	SESSID_SZ  = 32,
	USERID_SZ  = 16,
	NAME_SZ	   = 32,
	TOKEN_SZ   = 32,
	CHAT_MSGSZ = 1024
};

enum mix_rec {
	REC_DISABLED	= 0,
	REC_AUDIO	= 1,
	REC_VIDEO	= 2,
	REC_AUDIO_VIDEO = 3
};

enum user_event { USER_ADDED, USER_UPDATED, USER_DELETED, CHAT_ADDED };

typedef int(mix_rec_h)(const char *folder, bool enable);
typedef void(mix_disp_enable_h)(const char *device, bool enable);
typedef uint64_t(mix_time_rec_h)(void);

struct mix {
	struct list sessl;
	struct list chatl;
	uint16_t next_speaker_id;
	const struct mnat *mnat;
	const struct menc *menc;
	struct http_sock *httpsock;
	const char *www_path;
	char token_host[TOKEN_SZ];
	char token_guests[TOKEN_SZ];
	char token_listeners[TOKEN_SZ];
	char token_download[TOKEN_SZ];
	struct rtc_configuration pc_config;
	mix_rec_h *audio_rec_h;
	mix_rec_h *video_rec_h;
	mix_disp_enable_h *disp_enable_h;
	mix_time_rec_h *time_rec_h;
	enum mix_rec rec_state;
};

struct user {
	char id[USERID_SZ];
	char name[NAME_SZ];
	uint16_t speaker_id;
	bool speaker;
	bool host;
	bool video;
	bool audio;
	bool hand;
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
	bool connected; /* Websocket connected */
	struct media_track *maudio;
	struct media_track *mvideo;
	struct mix *mix;
};


/******************************************************************************
 * mix.c
 */
struct mix *slmix(void);
int slmix_init(void);
void slmix_config(char *file);
void slmix_close(void);
void slmix_set_audio_rec_h(struct mix *m, mix_rec_h *rec_h);
void slmix_set_video_rec_h(struct mix *m, mix_rec_h *rec_h);
void slmix_set_time_rec_h(struct mix *m, mix_time_rec_h *time_h);
void slmix_set_video_disp_h(struct mix *m, mix_disp_enable_h *disp_h);
void slmix_record(struct mix *m, enum mix_rec state);
void slmix_disp_enable(struct mix *m, const char *dev, bool enable);
enum mix_rec slmix_rec_state(struct mix *m);
const char *slmix_rec_state_name(enum mix_rec state);

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
void pc_close(struct session *sess);
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
void sl_ws_session_close(struct session *sess);


/******************************************************************************
 * external modules (amix, vmix etc.)
 * @TODO: convert to registered functions or shared header
 */
void amix_mute(char *device, bool mute, uint16_t id);