// test.c
// 

#include "parser.h"


int main(int argc, char* argv[]) {
	CppParser env;
	gchar* buf;
	gsize len;
	gint i;

	cpp_parser_init(&env, TRUE);

	g_file_get_contents("tee.cpp", &buf, &len, 0);
	if( buf ) {
		GTimer* timer;
		gdouble used;

		#define n 5000
		timer = g_timer_new();
		for(i=0; i<n; ++i)
			cpp_parser_test(&env, buf, len);
		used = g_timer_elapsed(timer, NULL);
		g_timer_destroy(timer);

		g_print("%f - %f\n", used, used/n);
		g_free(buf);

	} else {
		g_print("load file failed!\n");
	}

	cpp_parser_final(&env);

	return 0;
}

