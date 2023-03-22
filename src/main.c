#include <getopt.h>
#include <mix.h>


static const char *modv[] = {
	"ice",
	"dtls_srtp",

	/* audio */
	"amix",
	"opus",
	"auresamp",

	/* video */
	"vmix",
	"avcodec",
};

static char *config_file = NULL;


static void signal_handler(int sig)
{
	(void)sig;
	re_cancel();
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

	(void)re_printf("Mix v%s-%s"
			" Copyright (C) 2013 - 2023"
			" Sebastian Reimers\n\n",
			SLMIX_VERSION, slmix_git_revision());

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
			break;
		case 'v':
			log_enable_debug(true);
			break;
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


int main(int argc, char *const argv[])
{
	(void)argc;
	(void)argv;
	int err;
	struct config *config;
	struct mix *mix = slmix();

	const char *conf = "call_max_calls		10\n" /* SIP only */
			   "sip_verify_server	yes\n"
			   "audio_buffer		20-160\n"
			   "audio_buffer_mode	fixed\n"
			   "audio_silence		-35.0\n"
			   "jitter_buffer_type     fixed\n"
			   "jitter_buffer_wish     5\n"
			   "jitter_buffer_delay    5-10\n"
			   "opus_bitrate		64000\n"
			   "ice_policy		relay\n"
			   "video_size		1920x1080\n"
			   "video_bitrate		2000000\n"
			   "video_fps		25\n"
			   "avcodec_keyint		2\n"
			   "avcodec_h265enc	nil\n"
			   "avcodec_h265dec	nil\n"
			   "audio_txmode		thread\n";

	/*
	 * turn off buffering on stdout
	 */
	setbuf(stdout, NULL);

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
	config->audio.channels_play = 1;
	config->audio.channels_src  = 1;

	config->avt.rtcp_mux  = true;
	config->avt.rtp_stats = true;

	err = slmix_config(config_file);
	if (err)
		return err;

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

	err = slmix_init();
	if (err)
		return err;

	sl_ws_init();
	err = slmix_http_listen(&mix->httpsock, mix);
	if (err)
		return err;

	re_main(signal_handler);

	sl_ws_close();
	slmix_close();

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
