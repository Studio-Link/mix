#include <re.h>
#include <baresip.h>
#include "mix.h"

static const char *modv[] = {
	"ice",
	"dtls_srtp",

	/* audio */
	"opus",

	/* video */
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


int main(int argc, char *const argv[])
{
	(void)argc;
	(void)argv;
	int err;

	struct mix mix = {.sessl = LIST_INIT, .pc_config = {.offerer = false}};

	const char *conf = "call_max_calls	100\n"
			   "sip_verify_server	yes\n"
			   "audio_buffer	20-160\n"
			   "audio_buffer_mode	adaptive\n"
			   "audio_silence	-35.0\n"
			   "jitter_buffer_type	off\n"
			   "opus_bitrate	64000\n"
			   "ice_policy		relay\n";

	err = libre_init();
	if (err)
		return err;

	fd_setsize(-1);
	re_thread_async_init(4);

	(void)sys_coredump_set(true);

	err = conf_configure_buf((const uint8_t *)conf, str_len(conf));
	if (err) {
		warning("conf_configure failed: %m\n", err);
		return err;
	}

	err = baresip_init(conf_config());
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

	slmix_http_listen(&mix.httpsock, &mix);
	if (err)
		return err;

	re_main(signal_handler);

	mem_deref(mix.httpsock);

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
