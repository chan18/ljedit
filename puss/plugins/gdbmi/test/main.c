#include "../vdriver.h"

gboolean test_pause(MIVDriver* drv) {
	mi_vdriver_command_pause(drv);
	return FALSE;
}

gboolean test_continue(MIVDriver* drv) {
	mi_vdriver_command_continue(drv);
	return FALSE;
}

gboolean test_stop(MIVDriver* drv) {
	mi_vdriver_command_stop(drv);
	return FALSE;
}

int main(int argc, char* argv[]) {
	GMainLoop* main_loop;
	MIVDriver* drv;

	main_loop = g_main_loop_new(0, FALSE);

	drv = mi_vdriver_new();

	if( !mi_vdriver_command_run(drv) )
		return 1;

	g_timeout_add_seconds(5, (GSourceFunc)test_pause, drv);
	g_timeout_add_seconds(10, (GSourceFunc)test_continue, drv);
	g_timeout_add_seconds(15, (GSourceFunc)test_stop, drv);
	g_timeout_add_seconds(20, (GSourceFunc)g_main_loop_quit, main_loop);
	g_main_loop_run(main_loop);

	mi_vdriver_free(drv);
	g_main_loop_unref(main_loop);

	return 0;
}

