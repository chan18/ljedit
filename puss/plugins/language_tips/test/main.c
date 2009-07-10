// main.c
// 

#include "../cpp/guide.h"

int main(int argc, char* argv[]) {
	CppGuide* guide;

	g_mem_set_vtable(glib_mem_profiler_table);
	g_atexit(g_mem_profile);

	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);
	guide = cpp_guide_new(TRUE, TRUE);
	cpp_guide_free(guide);

	return 0;
}

