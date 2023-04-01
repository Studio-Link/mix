#include <time.h>
#include <unistd.h>
#include <mix.h>

extern const char *GIT_TAG;
extern const char *GIT_REV;
extern const char *GIT_BRANCH;


static struct mix mix = {.room		  = "main",
			 .sessl		  = LIST_INIT,
			 .pc_config	  = {.offerer = false},
			 .token_host	  = "",
			 .token_guests	  = "",
			 .token_listeners = "",
			 .token_download  = ""};

const char *slmix_git_version(void)
{
	return GIT_TAG;
}


const char *slmix_git_revision(void)
{
	return GIT_REV;
}


const char *slmix_git_branch(void)
{
	return GIT_BRANCH;
}


struct mix *slmix(void)
{
	return &mix;
}


static struct tmr tmr_room_update;
static uint64_t last_room_update = 0;
struct mbuf update_data;


void slmix_refresh_rooms(void *arg)
{
	struct mbuf mbkey, val;
	struct pl key;
	uint64_t current;
	void *cur;
	char *json = NULL;
	struct mbuf mjson;
	bool force = false;

	if (arg)
		force = *(bool *)arg;

	pl_set_str(&key, "up");

	slmix_db_get(slmix_db_rooms(), &key, &update_data);

	current = mbuf_read_u64(&update_data);
	mbuf_set_posend(&update_data, 0, 0);

	if (!force && current == last_room_update)
		goto out;

	last_room_update = current;

	mbuf_init(&mbkey);
	mbuf_init(&val);
	mbuf_init(&mjson);

	slmix_db_cur_open(&cur, slmix_db_rooms());

	while (slmix_db_cur_next(cur, &mbkey, &val) == 0) {
		if (str_cmp((char *)mbkey.buf, "up") == 0)
			continue;

		mbuf_printf(&mjson, "\"%s\": %s,", mbkey.buf, val.buf);

		mbuf_set_posend(&mbkey, 0, 0);
		mbuf_set_posend(&val, 0, 0);
	}

	re_sdprintf(&json, "{\"type\": \"rooms\", \"rooms\": {%b}}", mjson.buf,
		    mjson.end - 1);
	sl_ws_send_event_all(json);
	mem_deref(json);

	slmix_db_cur_close(cur);
	mbuf_reset(&mbkey);
	mbuf_reset(&val);
	mbuf_reset(&mjson);

out:
	tmr_start(&tmr_room_update, 500, slmix_refresh_rooms, NULL);
}


int slmix_update_room(void)
{
	char str[128] = {0};
	struct pl key;
	int err;

	re_snprintf(str, sizeof(str),
		    "{\"url\": \"%s\", \"listeners\": %u, \"show\": %d}",
		    mix.url, list_count(sl_ws_list()), mix.show);
	pl_set_str(&key, mix.room);

	err = slmix_db_put(slmix_db_rooms(), &key, str, str_len(str) + 1);
	if (err)
		return err;

	err = slmix_db_up(slmix_db_rooms());
	if (err)
		return err;

	return err;
}


int slmix_init(void)
{
	struct pl srv;
	int err;
	pl_set_str(&srv, "turn:195.201.63.86:3478");

	err = stunuri_decode(&mix.pc_config.ice_server, &srv);
	if (err) {
		warning("mix: invalid iceserver '%r' (%m)\n", &srv, err);
		return err;
	}

	mix.pc_config.stun_user	 = "turn200301";
	mix.pc_config.credential = "choh4zeem3foh1";

	mix.mnat = mnat_find(baresip_mnatl(), "ice");
	if (!mix.mnat) {
		warning("mix: medianat 'ice' not found\n");
		return ENOENT;
	}

	mix.menc = menc_find(baresip_mencl(), "dtls_srtp");
	if (!mix.menc) {
		warning("mix: mediaenc 'dtls-srtp' not found\n");
		return ENOENT;
	}

	err = slmix_db_init();
	if (err)
		return err;

	err = sl_httpc_init();
	if (err)
		return err;

	mbuf_init(&update_data);
	tmr_init(&tmr_room_update);
	tmr_start(&tmr_room_update, 100, slmix_refresh_rooms, NULL);

	err = slmix_update_room();

	return err;
}


int slmix_config(char *file)
{
	struct conf *conf;
	int err;

	if (!file) {
		warning("slmix_config: no config file\n");
		return EINVAL;
	}

	err = conf_alloc(&conf, file);
	if (err)
		return ENOMEM;

	conf_get_str(conf, "mix_room", mix.room, sizeof(mix.room));
	conf_get_str(conf, "mix_url", mix.url, sizeof(mix.url));
	err = conf_get_bool(conf, "mix_show", &mix.show);
	if (err) {
		mix.show = true;
	}

	conf_get_str(conf, "mix_token_host", mix.token_host,
		     sizeof(mix.token_host));

	conf_get_str(conf, "mix_token_guests", mix.token_guests,
		     sizeof(mix.token_guests));

	conf_get_str(conf, "mix_token_listeners", mix.token_listeners,
		     sizeof(mix.token_listeners));

	conf_get_str(conf, "mix_token_download", mix.token_download,
		     sizeof(mix.token_download));

	err = conf_get_str(conf, "mix_path", mix.path, sizeof(mix.path));
	if (err) {
		if (!getcwd(mix.path, sizeof(mix.path))) {
			warning("slmix_config: getcwd failed\n");
			err = errno;
			goto out;
		}
		err = 0;
	}

	info("slmix_config path: %s\n", mix.path);

out:
	mem_deref(conf);

	return err;
}


void slmix_close(void)
{
	struct pl key;

	info("slmix close\n");

	pl_set_str(&key, mix.room);

#if 0 /* permanent rooms */
	slmix_db_del(slmix_db_rooms(), &key);
	slmix_db_up(slmix_db_rooms());
#endif

	tmr_cancel(&tmr_room_update);
	mbuf_reset(&update_data);

	list_flush(&mix.sessl);
	list_flush(&mix.chatl);

	mix.httpsock		 = mem_deref(mix.httpsock);
	mix.pc_config.ice_server = mem_deref(mix.pc_config.ice_server);

	slmix_db_close();
	sl_httpc_close();
}


void slmix_set_audio_rec_h(struct mix *m, mix_rec_h *rec_h)
{
	if (!m)
		return;

	m->audio_rec_h = rec_h;
}


void slmix_set_video_rec_h(struct mix *m, mix_rec_h *rec_h)
{
	if (!m)
		return;

	m->video_rec_h = rec_h;
}


void slmix_set_video_disp_h(struct mix *m, mix_disp_enable_h *disp_h)
{
	if (!m)
		return;

	m->disp_enable_h = disp_h;
}


static int timestamp_print(struct re_printf *pf, const struct tm *tm)
{
	if (!tm)
		return 0;

	return re_hprintf(pf, "%d-%02d-%02d-%02d-%02d-%02d",
			  1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
			  tm->tm_hour, tm->tm_min, tm->tm_sec);
}


const char *slmix_rec_state_name(enum mix_rec state)
{
	switch (state) {

	case REC_DISABLED:
		return "disabled";
	case REC_AUDIO:
		return "audio only";
	case REC_VIDEO:
		return "video only";
	case REC_AUDIO_VIDEO:
		return "audio + video";
	default:
		return "??";
	}
}


enum mix_rec slmix_rec_state(struct mix *m)
{
	return m ? m->rec_state : REC_DISABLED;
}


void slmix_record(struct mix *m, enum mix_rec state)
{
	struct tm tm;
	time_t now	     = time(NULL);
	char tmp[PATH_SZ]    = {0};
	char folder[PATH_SZ] = {0};
	int err		     = 0;

	if (!m || !m->video_rec_h || !m->audio_rec_h) {
		warning("slmix: record init state %s failed\n",
			slmix_rec_state_name(state));
		return;
	}

	if (state == REC_DISABLED) {
		err = m->video_rec_h(NULL, false);
		err |= m->audio_rec_h(NULL, false);

		if (err) {
			warning("slmix: record disable %s (%m) failed\n",
				slmix_rec_state_name(state), err);
		}

		info("slmix: record stopped\n");
		goto out;
	}

	localtime_r(&now, &tm);

	(void)re_snprintf(tmp, sizeof(tmp), "webui/public/download/%s",
			  m->token_download);
	fs_mkdir(tmp, 0755);

	(void)re_snprintf(tmp, sizeof(tmp), "webui/public/download/%s/%s",
			  m->token_download, m->room);
	fs_mkdir(tmp, 0755);

	(void)re_snprintf(folder, sizeof(folder), "%s/%H", tmp,
			  timestamp_print, &tm);
	fs_mkdir(folder, 0755);


	if (state & REC_VIDEO) {
		err = m->video_rec_h(folder, true);
	}

	if (state & REC_AUDIO) {
		err |= m->audio_rec_h(folder, true);
	}

	if (err) {
		warning("slmix: record enable %s (%m) failed\n",
			slmix_rec_state_name(state), err);
		m->rec_state = REC_DISABLED;
		return;
	}

	info("slmix: record %s started\n", slmix_rec_state_name(state));

out:
	m->rec_state = state;
}


uint32_t slmix_disp_enable(struct mix *m, const char *dev, bool enable)
{
	if (!m || !m->disp_enable_h)
		return 0;

	return m->disp_enable_h(dev, enable);
}


void slmix_set_time_rec_h(struct mix *m, mix_time_rec_h *time_h)
{
	if (!m)
		return;

	m->time_rec_h = time_h;
}


void slmix_set_talk_detect_h(struct mix *m, mix_talk_detect_h *talk_h)
{
	if (!m)
		return;

	m->talk_detect_h = talk_h;
}
