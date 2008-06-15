// test.c
// 

#include "parser.h"


int main(int argc, char* argv[]) {
	CppParser env;
	gchar* buf;
	gsize len;
	gint i;
	CppFile* file;

	cpp_parser_init(&env, TRUE);

	g_file_get_contents("tee.cpp", &buf, &len, 0);
	if( buf ) {
		GTimer* timer;
		gdouble used;

		#define n 5000
		timer = g_timer_new();
		for(i=0; i<n; ++i) {
			file = cpp_parser_parse(&env, "tee.cpp", 7, buf, len);
			cpp_file_clear(file);
			g_free(file);
		}
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

