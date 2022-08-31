#include <re.h>


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

	re_main(signal_handler);

	re_thread_async_close();
	tmr_debug();
	libre_close();
	mem_debug();

	return 0;
}
