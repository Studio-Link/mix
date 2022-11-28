#include "mix.h"

static const char *modv[] = {
	"ice",
	"dtls_srtp",

	/* audio */
	"aumix",
	"opus",

	/* video */
	"vidmix",
	/* "fakevideo", */
	"avcodec",
};

static void signal_handler(int sig)
{
	(void)sig;
	re_cancel();
}

static int slmix_init(struct mix *mix)
{
	struct pl srv;
	int err;
	pl_set_str(&srv, "turn:195.201.63.86:3478");

	err = stunuri_decode(&mix->pc_config.ice_server, &srv);
	if (err) {
		warning("mix: invalid iceserver '%r' (%m)\n", &srv, err);
		return err;
	}

	mix->pc_config.stun_user  = "turn200301";
	mix->pc_config.credential = "choh4zeem3foh1";

	mix->mnat = mnat_find(baresip_mnatl(), "ice");
	if (!mix->mnat) {
		warning("mix: medianat 'ice' not found\n");
		return ENOENT;
	}

	mix->menc = menc_find(baresip_mencl(), "dtls_srtp");
	if (!mix->menc) {
		warning("mix: mediaenc 'dtls-srtp' not found\n");
		return ENOENT;
	}

	return 0;
}


static void slmix_close(struct mix *mix)
{
	list_flush(&mix->sessl);

	mix->httpsock		  = mem_deref(mix->httpsock);
	mix->pc_config.ice_server = mem_deref(mix->pc_config.ice_server);
}


int main(int argc, char *const argv[])
{
	(void)argc;
	(void)argv;
	int err;
	struct config *config;

	struct mix mix = {.sessl = LIST_INIT, .pc_config = {.offerer = false}};

	const char *conf = "call_max_calls	10\n" /* SIP only */
			   "sip_verify_server	yes\n"
			   "audio_buffer	20-160\n"
			   "audio_buffer_mode	adaptive\n"
			   "audio_silence	-35.0\n"
			   "jitter_buffer_type	off\n"
			   "opus_bitrate	64000\n"
			   "ice_policy		relay\n"
			   "video_size		1920x1080\n"
			   "video_bitrate	2097152\n" /* 2 MBit/s */
			   "video_fps		25\n"
			   "rtp_timeout		10\n";

	err = libre_init();
	if (err)
		return err;

	fd_setsize(-1);
	re_thread_async_init(4);
	/* log_enable_debug(true); */

	(void)sys_coredump_set(true);

	err = conf_configure_buf((const uint8_t *)conf, str_len(conf));
	if (err) {
		warning("conf_configure failed: %m\n", err);
		return err;
	}

	config = conf_config();
	config->net.use_linklocal = false;

	err = baresip_init(config);
	if (err) {
		warning("baresip_init failed (%m)\n", err);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(modv); i++) {

		err = module_load(".", modv[i]);
		if (err) {
			warning("could not pre-load module"
				" '%s' (%m)\n",
				modv[i], err);
		}
	}

	err = slmix_init(&mix);
	if (err)
		return err;

	sl_ws_init();
	err = slmix_http_listen(&mix.httpsock, &mix);
	if (err)
		return err;

	re_main(signal_handler);

	sl_ws_close();
	slmix_close(&mix);

	module_app_unload();
	conf_close();

	baresip_close();
	mod_close();

	re_thread_async_close();
	tmr_debug();
	libre_close();
	mem_debug();

	return 0;
}
