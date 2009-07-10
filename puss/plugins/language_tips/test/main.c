// main.c
// 

#include <vld.h>
#include "../cpp/guide.h"

int main(int argc, char* argv[]) {
	int i;
	CppFile* file;
	CppGuide* guide;

	g_mem_set_vtable(glib_mem_profiler_table);
	g_atexit(g_mem_profile);

	g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);

	file = 0;
	guide = cpp_guide_new(TRUE, FALSE);
	cpp_guide_include_paths_set(guide, "$pkg-config --cflags gtk+-2.0");

	for( i=0; i<(argc>1 ? 100 : 1); ++i ) {
		//file = cpp_guide_parse(guide, "D:\\puss\\plugins\\language_tips\\cpp\\guide.c", -1, FALSE);
		file = cpp_guide_parse(guide, "C:\\gtk\\include\\gtk-2.0\\gtk\\gtk.h", -1, TRUE);
		
		if( file )
			cpp_file_unref(file);
	}
	cpp_guide_free(guide);

	return 0;
}

