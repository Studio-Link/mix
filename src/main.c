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
	"avcodec",
	"vmix",
};

static char *config_file = NULL;
static char *config_listen = NULL;


const char *slmix_config_listen(void)
{
	if (!config_listen)
		return "127.0.0.1";
	return config_listen;
}


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
				 "\t-l          Listen IP\n"
				 "\t-v		Verbose debug\n");
}


static int slmix_getopt(int argc, char *const argv[])
{
#ifdef HAVE_GETOPT
	int index		= 0;
	struct option options[] = {{"config", required_argument, 0, 'c'},
				   {"help", 0, 0, 'h'},
				   {"verbose", 0, 0, 'v'},
				   {"listen", required_argument, 0, 'l'},
				   {0, 0, 0, 0}};
	(void)re_printf(
		"   _____ __            ___         __    _       __\n"
		"  / ___// /___  ______/ (_)___    / /   (_)___  / /__\n"
		"  \\__ \\/ __/ / / / __  / / __ \\  / /   / / __ \\/ //_/\n"
		" ___/ / /_/ /_/ / /_/ / / /_/ / / /___/ / / / / ,<\n"
		"/____/\\__/\\__,_/\\__,_/_/\\____(_)_____/_/_/ /_/_/|_|"
		"\n");

	(void)re_printf("Mix v%s-%s"
			" Copyright (C) 2013 - 2024"
			" Sebastian Reimers\n\n",
			SLMIX_VERSION, slmix_git_revision());

	for (;;) {
		const int c =
			getopt_long(argc, argv, "c:hvl:", options, &index);
		if (c < 0)
			break;

		switch (c) {

		case 'c':
			if (!fs_isfile(optarg)) {
				warning("config not found: %s\n", optarg);
				return EINVAL;
			}
			str_dup(&config_file, optarg);
			break;
		case 'h':
			usage();
			return -2;
		case 'v':
			log_enable_debug(true);
			break;
		case 'l':
			str_dup(&config_listen, optarg);
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

	const char *conf =
/*		"sip_listen		0.0.0.0:5060\n" */
		"call_max_calls		10\n" /* SIP incoming only */
		"sip_verify_server	yes\n"
		"audio_buffer		40-100\n"
		"audio_buffer_mode	fixed\n"
		"audio_silence		-35.0\n"
		"audio_jitter_buffer_type	adaptive\n"
		"audio_jitter_buffer_ms	60-300\n"
		"video_jitter_buffer_type	fixed\n"
		"video_jitter_buffer_ms	100-300\n"
		"video_jitter_buffer_size 500\n"
		"opus_bitrate		64000\n"
		"ice_policy		all\n"
		"video_size		1920x1080\n"
		"video_bitrate		4000000\n"
		"video_sendrate		10000000\n" /* max burst send */
		"video_burst_bit	1000000\n"  /* max burst send */
		"video_fps		24\n"
		"avcodec_keyint		10\n"
		"avcodec_h265enc	nil\n"
		"avcodec_h265dec	nil\n"
#if 0
		"videnc_format		nv12\n"
		"avcodec_h264enc	h264_nvenc\n"
		"avcodec_h264dec	h264\n"
		"avcodec_hwaccel	cuda\n"
#endif
		"audio_txmode		thread\n";

	/*
	 * turn off buffering on stdout
	 */
	setbuf(stdout, NULL);

	err = libre_init();
	if (err)
		return err;

#ifdef RE_TRACE_ENABLED
	err = re_trace_init("re_trace.json");
	if (err)
		return err;
#endif

	err = slmix_getopt(argc, argv);
	if (err)
		return err;

	fd_setsize(-1);
	re_thread_async_init(8);

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

	err = ua_init("StudioLinkMix", true, true, true);
	if (err) {
		warning("ua_init failed (%m)\n", err);
		return err;
	}

	for (size_t i = 0; i < RE_ARRAY_SIZE(modv); i++) {

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

	ua_stop_all(true);
	ua_close();

	module_app_unload();
	conf_close();

	config_file = mem_deref(config_file);
	config_listen = mem_deref(config_listen);

	baresip_close();
	mod_close();

	re_thread_async_close();
#ifdef RE_TRACE_ENABLED
	re_trace_close();
#endif
	tmr_debug();
	libre_close();
	mem_debug();

	return 0;
}
