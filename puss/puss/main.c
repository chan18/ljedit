// main.c

#include <libintl.h>
#include <gtk/gtk.h>

#include <string.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"
#include "DndOpen.h"

#ifdef G_OS_WIN32
	#include <Windows.h>

	gchar* find_module_filepath(const char* argv0) {
		gchar buf[4096];
		int len = GetModuleFileNameA(0, buf, 4096);
		return g_locale_to_utf8(buf, len, NULL, NULL, NULL);
	}

#else
	gchar* find_module_filepath(const char* argv0) {
		gchar* pwd;
		gchar* prj;
		gchar* realpath;
		gchar* filepath = g_find_program_in_path(argv0);

		if( !filepath ) {
			g_printerr(_("ERROR : can not find puss in $PATH\n"));
			return 0;
		}

		if( !g_path_is_absolute(filepath) ) {
			pwd = g_get_current_dir();
			prj = g_build_filename(pwd, filepath, NULL);
			g_free(pwd);
			g_free(filepath);
			filepath = prj;
		}

		if( g_file_test(filepath, G_FILE_TEST_IS_SYMLINK) ) {
			realpath = g_file_read_link(filepath, 0);
			g_free(filepath);
			filepath = realpath;
		}

		if( !g_file_test(filepath, G_FILE_TEST_EXISTS) ) {
			g_printerr(_("ERROR : can not find puss directory!\n"));
			g_printerr(_("        can not use indirect search path in $PATH!\n"));
			g_free(filepath);
			return 0;
		}

		return filepath;
	}

#endif

void open_arg1_file(const char* argv1) {
	gchar* pwd;
	gchar* tmp;
	gssize len = (gssize)strlen(argv1);
	gchar* filepath = g_locale_to_utf8(argv1, len, NULL, NULL, NULL);
	if( !g_path_is_absolute(filepath) ) {
		pwd = g_get_current_dir();
		tmp = g_build_filename(pwd, filepath, NULL);
		g_free(pwd);
		g_free(filepath);
		filepath = tmp;
	}

	if( g_file_test(filepath, G_FILE_TEST_EXISTS) )
		puss_doc_open(filepath, -1, -1, TRUE);

	g_free(filepath);
}

int main(int argc, char* argv[]) {
	gchar* filepath;
	gboolean res;

	// can not work with valgrind
	// 
	// g_mem_set_vtable(glib_mem_profiler_table);
	// g_atexit(g_mem_profile);

	// for debug
	//g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);

	g_thread_init(NULL);

	//gdk_threads_init();

	gtk_init(&argc, &argv);

	filepath = find_module_filepath(argv[0]);
	res = puss_create(filepath);
	g_free(filepath);

	puss_dnd_open_support();

	if( argc==2 )
		open_arg1_file(argv[1]);

	if( res ) {
		puss_run();
		puss_destroy();
	}

	return 0;
}

