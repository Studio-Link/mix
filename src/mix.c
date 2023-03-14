#include <time.h>
#include <mix.h>

static struct mix mix = {.sessl		  = LIST_INIT,
			 .pc_config	  = {.offerer = false},
			 .token_host	  = "",
			 .token_guests	  = "",
			 .token_listeners = "",
			 .token_download  = ""};


struct mix *slmix(void)
{
	return &mix;
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

	return 0;
}


void slmix_config(char *file)
{
	struct conf *conf;

	if (!file)
		return;

	if (0 != conf_alloc(&conf, file))
		return;

	conf_get_str(conf, "mix_token_host", mix.token_host,
		     sizeof(mix.token_host));

	conf_get_str(conf, "mix_token_guests", mix.token_guests,
		     sizeof(mix.token_guests));

	conf_get_str(conf, "mix_token_listeners", mix.token_listeners,
		     sizeof(mix.token_listeners));

	conf_get_str(conf, "mix_token_download", mix.token_download,
		     sizeof(mix.token_download));

	mem_deref(conf);
}


void slmix_close(void)
{
	list_flush(&mix.sessl);
	list_flush(&mix.chatl);

	mix.httpsock		 = mem_deref(mix.httpsock);
	mix.pc_config.ice_server = mem_deref(mix.pc_config.ice_server);
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
	time_t now	 = time(NULL);
	char folder[256] = {0};
	int err		 = 0;

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

	(void)re_snprintf(folder, sizeof(folder), "webui/public/download/%s",
			  m->token_download);
	fs_mkdir(folder, 0755);

	(void)re_snprintf(folder, sizeof(folder), "%s/%H", folder,
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


void slmix_disp_enable(struct mix *m, const char *dev, bool enable)
{
	if (!m || !m->disp_enable_h)
		return;

	m->disp_enable_h(dev, enable);
}


void slmix_set_time_rec_h(struct mix *m, mix_time_rec_h *time_h)
{
	if (!m)
		return;

	m->time_rec_h = time_h;
}
