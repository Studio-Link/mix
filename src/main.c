#include <re.h>

static struct http_sock *httpsock = NULL;


static int slmix_http_listen(struct http_sock **sock)
{
	int err;
	struct sa srv;

	if (!sock)
		return EINVAL;

	err = sa_set_str(&srv, "127.0.0.1", 9999);
	if (err)
		return err;

	re_printf("listen webui: http://%J\n", &srv);
	err = http_listen(sock, &srv, http_req_handler, NULL);

	return err;
}



static void signal_handler(int sig)
{
	(void)sig;
	re_cancel();
}


int main(int argc, char *const argv[])
{
	(void)argc;
	(void)argv;

	libre_init();
	fd_setsize(-1);
	re_thread_async_init(4);

	slmix_http_listen(&httpsock);

	re_main(signal_handler);

	re_thread_async_close();
	tmr_debug();
	libre_close();
	mem_debug();

	return 0;
}
