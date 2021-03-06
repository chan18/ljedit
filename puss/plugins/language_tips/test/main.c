// main.c
// 

#include "../cpp/guide.h"

int main(int argc, char* argv[]) {
	int i;
	CppFile* file;
	CppGuide* guide;
	gpointer kws;

	g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);

	//g_mem_set_vtable(glib_mem_profiler_table);
	//g_atexit(g_mem_profile);

	file = 0;
	for( i=0; i<(argc>1 ? 100 : 1); ++i ) {
		guide = cpp_guide_new(TRUE, FALSE);
		cpp_guide_include_paths_set(guide, "$pkg-config --cflags gtk+-2.0");
		//file = cpp_guide_parse(guide, "D:\\puss\\plugins\\language_tips\\cpp\\guide.c", -1, FALSE);
		//file = cpp_guide_parse(guide, "C:\\gtk\\include\\gtk-2.0\\gtk\\gtk.h", -1, TRUE);
		file = cpp_guide_parse(guide, "/home/louis/puss/puss/main.c", -1, TRUE);
		
		if( file )
			cpp_file_unref(file);
		
		cpp_guide_free(guide);
	}

	return 0;
}

