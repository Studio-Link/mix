#include <re.h>
#include <baresip.h>

#define METRICS_URL "http://127.0.0.1:9091/metrics/job/rtc"

enum {
	ROOM_SZ	   = 128,
	SESSID_SZ  = 32,
	USERID_SZ  = 16,
	NAME_SZ	   = 32,
	TOKEN_SZ   = 33,
	CHAT_MSGSZ = 1024,
	PATH_SZ	   = 256,
	URL_SZ	   = 256,
};

enum mix_rec {
	REC_DISABLED	= 0,
	REC_AUDIO	= 1,
	REC_VIDEO	= 2,
	REC_AUDIO_VIDEO = 3
};

enum user_event { USER_ADDED, USER_UPDATED, USER_DELETED, CHAT_ADDED };

typedef int(mix_rec_h)(const char *folder, bool enable);
typedef uint32_t(mix_disp_enable_h)(const char *device, bool enable);
typedef uint64_t(mix_time_rec_h)(void);
typedef uint16_t(mix_talk_detect_h)(void);

struct mix {
	char room[ROOM_SZ];
	char url[URL_SZ];
	bool show;
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
	mix_talk_detect_h *talk_detect_h;
	enum mix_rec rec_state;
	char path[PATH_SZ];
};

struct user {
	char id[USERID_SZ];
	int pidx; /* Video position index, 0 => disabled */
	char name[NAME_SZ];
	uint16_t speaker_id;
	bool speaker;
	bool host;
	bool video;
	bool audio;
	bool hand;
	bool solo;
	bool calling;
};

struct chat {
	struct le le;
	uint64_t time;
	struct user *user;
	char message[CHAT_MSGSZ];
};

struct source_pc {
	int32_t id;
	struct le le;
	struct peer_connection *pc;
	struct session *sess;
	struct call *call;
	char source_dev[128];
	char dev[128];
};

struct session {
	struct le le;
	bool auth;
	struct peer_connection *pc;
	struct list source_pcl;
	struct http_conn *conn_pending;
	char id[SESSID_SZ]; /* Keep secret */
	struct user *user;
	bool connected; /* SIP or Websocket connected */
	struct media_track *maudio;
	struct media_track *mvideo;
	struct mix *mix;
	struct call *call;
};


/******************************************************************************
 * main.c
 */
const char *slmix_config_listen(void);


/******************************************************************************
 * mix.c
 */
const char *slmix_git_version(void);
const char *slmix_git_revision(void);
const char *slmix_git_branch(void);
struct mix *slmix(void);
int slmix_init(void);
int slmix_config(char *file);
void slmix_close(void);
void slmix_set_audio_rec_h(struct mix *m, mix_rec_h *rec_h);
void slmix_set_video_rec_h(struct mix *m, mix_rec_h *rec_h);
void slmix_set_time_rec_h(struct mix *m, mix_time_rec_h *time_h);
void slmix_set_talk_detect_h(struct mix *m, mix_talk_detect_h *talk_h);
void slmix_set_video_disp_h(struct mix *m, mix_disp_enable_h *disp_h);
void slmix_record(struct mix *m, enum mix_rec state);
uint32_t slmix_disp_enable(struct mix *m, const char *dev, bool enable);
enum mix_rec slmix_rec_state(struct mix *m);
const char *slmix_rec_state_name(enum mix_rec state);
void slmix_refresh_rooms(void *arg);
int slmix_update_room(void);

/******************************************************************************
 * sip.c
 */
int slmix_sip_init(struct mix *mix);
int slmix_sip_close(void);


/******************************************************************************
 * avatar.c
 */
int avatar_save(struct session *sess, struct http_conn *conn,
		const struct http_msg *msg, re_async_h *cb, void *arg);
int avatar_delete(struct session *sess);


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
int slmix_session_user_updated(struct session *sess);
void slmix_session_video(struct session *sess, bool enable);
void slmix_session_video_solo(struct user *user, bool enable);
int slmix_session_speaker(struct session *sess, bool enable);
int slmix_session_new(struct mix *mix, struct session **sessp,
		      const struct http_msg *msg);
int slmix_session_auth(struct mix *mix, struct session *sess,
		       const struct http_msg *msg);
int slmix_session_alloc(struct session **sessp, struct mix *mix,
			const struct pl *sess_id, const struct pl *user_id,
			const struct pl *name, bool host, bool speaker);
int slmix_session_start(struct session *sess,
			const struct rtc_configuration *pc_config,
			const struct mnat *mnat, const struct menc *menc);
struct session *slmix_session_lookup_hdr(const struct list *sessl,
					 const struct http_msg *msg);
struct session *slmix_session_lookup(const struct list *sessl,
				     const struct pl *sessid);
struct session *slmix_session_lookup_user_id(const struct list *sessl,
					     const struct pl *user_id);
void slmix_session_close(struct session *sess, int err);
void http_sreply(struct http_conn *conn, uint16_t scode, const char *reason,
		 const char *ctype, const char *fmt, size_t size,
		 struct session *sess);
int slmix_session_save(struct session *sess);


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
int sl_ws_open(struct http_conn *httpc, const struct http_msg *msg,
	       struct mix *mix, struct session *sess);
void sl_ws_send_event(struct session *sess, char *json);
void sl_ws_send_event_self(struct session *sess, char *json);
void sl_ws_send_event_host(char *json);
void sl_ws_send_event_all(char *json);
void sl_ws_dummyh(const struct websock_hdr *hdr, struct mbuf *mb, void *arg);
void sl_ws_users_auth(const struct websock_hdr *hdr, struct mbuf *mb,
		      void *arg);
void sl_ws_session_close(struct session *sess);
struct list *sl_ws_list(void);


/******************************************************************************
 * db.c
 */
int slmix_db_up(unsigned int dbi);
int slmix_db_cur_open(void **cur, unsigned int dbi);
int slmix_db_cur_next(void *cur, struct mbuf *key, struct mbuf *data);
int slmix_db_cur_close(void *cur);
int slmix_db_get(unsigned int dbi, const struct pl *key, struct mbuf *data);
int slmix_db_put(unsigned int dbi, const struct pl *key, void *val, size_t sz);
int slmix_db_del(unsigned int dbi, const struct pl *key);
unsigned int slmix_db_sess(void);
unsigned int slmix_db_rooms(void);
int slmix_db_init(void);
void slmix_db_close(void);


/******************************************************************************
 * http_client.c
 */
enum sl_httpc_met {
	SL_HTTP_GET,
	SL_HTTP_POST,
	SL_HTTP_PUT,
	SL_HTTP_PATCH,
	SL_HTTP_DELETE
};

struct sl_httpconn {
	struct http_reqconn *conn;
	http_resp_h *slresph;
	int redirects;
	void *arg;
};

int sl_httpc_init(void);
void sl_httpc_close(void);
int sl_httpc_alloc(struct sl_httpconn **conn, http_resp_h *resph,
		   http_data_h *datah, void *arg);
int sl_httpc_req(struct sl_httpconn *conn, enum sl_httpc_met sl_met,
		 const char *url, struct mbuf *body);


/******************************************************************************
 * external modules (amix, vmix etc.)
 * @TODO: convert to registered functions or shared header
 */
void amix_mute(const char *device, bool mute, uint16_t id);
void vmix_disp_focus(const char *device);
void vmix_disp_solo(const char *device);


/******************************************************************************
 * source.c
 */
int slmix_source_alloc(struct source_pc **srcp, struct session *sess,
		       const char *dev);
void slmix_source_deref(struct mix *mix, const struct call *call,
			const char *dev);
int slmix_source_start(struct source_pc *src, struct mix *mix);
int slmix_source_append_all(struct mix *mix, struct call *call,
			    const char *dev);
int slmix_handle_ice_candidate(struct peer_connection *pc,
			       const struct odict *od);

/******************************************************************************
 * stats.c
 */
int slmix_stats_init(void);
void slmix_stats_close(void);

/******************************************************************************
 * social.c
 */
int social_request(struct http_conn *conn, const struct http_msg *msg,
		   struct session *sess);
