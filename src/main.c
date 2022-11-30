#include <getopt.h>
#include "mix.h"

static const char *modv[] = {
	"ice",
	"dtls_srtp",

	/* audio */
	"aumix",
	"opus",

	/* video */
	"vidmix",
	"avcodec",
};

static char *config_file = NULL;

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


static void usage(void)
{
	(void)re_fprintf(stderr, "Usage: slmix [options]\n"
				 "options:\n"
				 "\t-h		Help\n"
				 "\t-c --config	Load config file\n"
				 "\t-v		Verbose debug\n");
}


static int slmix_getopt(int argc, char *const argv[])
{
#ifdef HAVE_GETOPT
	int index		= 0;
	struct option options[] = {{"config", required_argument, 0, 'c'},
				   {"help", 0, 0, 'h'},
				   {0, 0, 0, 0}};
	(void)re_printf(
		"   _____ __            ___         __    _       __\n"
		"  / ___// /___  ______/ (_)___    / /   (_)___  / /__\n"
		"  \\__ \\/ __/ / / / __  / / __ \\  / /   / / __ \\/ //_/\n"
		" ___/ / /_/ /_/ / /_/ / / /_/ / / /___/ / / / / ,<\n"
		"/____/\\__/\\__,_/\\__,_/_/\\____(_)_____/_/_/ /_/_/|_|"
		"\n");

	(void)re_printf("Mix v%s"
			" Copyright (C) 2013 - 2022"
			" Sebastian Reimers\n\n",
			SLMIX_VERSION);

	for (;;) {
		const int c = getopt_long(argc, argv, "vhc:", options, &index);
		if (c < 0)
			break;

		switch (c) {

		case 'h':
			usage();
			return -2;
		case 'c':
			if (!fs_isfile(optarg)) {
				warning("config not found: %s\n", optarg);
				return EINVAL;
			}
			str_dup(&config_file, optarg);
			return 0;
		case 'v':
			log_enable_debug(true);
			return 0;
		default:
			usage();
			return EINVAL;
		}
	}
#else
	(void)argc;
	(void)argv;
#endif

	return 0;
}


static void slmix_config(struct mix *mix)
{
	struct conf *conf;

	if (!mix || !config_file)
		return;

	if (0 != conf_alloc(&conf, config_file))
		return;

	conf_get_str(conf, "mix_token_host", mix->token_host,
		     sizeof(mix->token_host));

	conf_get_str(conf, "mix_token_guests", mix->token_guests,
		     sizeof(mix->token_guests));

	conf_get_str(conf, "mix_token_listeners", mix->token_listeners,
		     sizeof(mix->token_listeners));

	conf_get_str(conf, "mix_token_download", mix->token_download,
		     sizeof(mix->token_download));

	mem_deref(conf);
}


int main(int argc, char *const argv[])
{
	(void)argc;
	(void)argv;
	int err;
	struct config *config;
	struct mix mix = {.sessl	   = LIST_INIT,
			  .pc_config	   = {.offerer = false},
			  .token_host	   = "",
			  .token_guests	   = "",
			  .token_listeners = "",
			  .token_download  = ""};

	const char *conf =
		"call_max_calls		10\n" /* SIP only */
		"sip_verify_server	yes\n"
		"audio_buffer		20-160\n"
		"audio_buffer_mode	adaptive\n"
		"audio_silence		-35.0\n"
		"jitter_buffer_type	off\n"
		"opus_bitrate		64000\n"
		"ice_policy		relay\n"
		"video_size		1920x1080\n"
		"video_bitrate		1572864\n" /* 1.5 MBit/s */
		"video_fps		25\n"
		"avcodec_keyint		10\n"
		"rtp_timeout		10\n";

	err = libre_init();
	if (err)
		return err;

	err = slmix_getopt(argc, argv);
	if (err)
		return err;

	fd_setsize(-1);
	re_thread_async_init(4);

	(void)sys_coredump_set(true);

	err = conf_configure_buf((const uint8_t *)conf, str_len(conf));
	if (err) {
		warning("conf_configure_buf failed: %m\n", err);
		return err;
	}

	config = conf_config();

	config->net.use_linklocal = false;

	config->audio.srate_play    = 48000;
	config->audio.srate_src	    = 48000;
	config->audio.channels_play = 2;
	config->audio.channels_src  = 2;

	slmix_config(&mix);

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

	config_file = mem_deref(config_file);

	baresip_close();
	mod_close();

	re_thread_async_close();
	tmr_debug();
	libre_close();
	mem_debug();

	return 0;
}
