// main.c
// 

#include "../cpp/guide.h"

int main(int argc, char* argv[]) {
	int i;
	CppFile* file;
	CppGuide* guide;

	g_mem_set_vtable(glib_mem_profiler_table);
	g_atexit(g_mem_profile);

	for( i=0; i<(argc>1 ? 100 : 1); ++i ) {
		guide = cpp_guide_new(TRUE, TRUE);
		file = cpp_guide_parse(guide, "D:\\puss\\plugins\\language_tips\\debug\\test.c", -1, FALSE);
		if( file )
			cpp_file_unref(file);
		cpp_guide_free(guide);
	}

	return 0;
}

