// main.c

#include <libintl.h>
#include <gtk/gtk.h>

#include <string.h>

#include "Puss.h"
#include "Utils.h"

#ifdef G_OS_WIN32
	#include <Windows.h>

	gchar* find_module_filepath(const char* argv0) {
		gchar buf[4096];
		int len = GetModuleFileNameA(0, buf, 4096);
		return g_locale_to_utf8(buf, len, NULL, NULL, NULL);
	}

#else
	gchar* find_module_filepath(const char* argv0) {
		gchar* filepath = g_find_program_in_path(argv0);

		if( !filepath ) {
			g_printerr(_("ERROR : can not find puss in $PATH\n"));
			return 0;
		}

		if( !g_path_is_absolute(filepath) ) {
			gchar* pwd = g_get_current_dir();
			gchar* prj =  g_build_filename(pwd, filepath, NULL);
			g_free(pwd);
			g_free(filepath);
			filepath = prj;
		}

		if( g_file_test(filepath, G_FILE_TEST_IS_SYMLINK) ) {
			gchar* realpath = g_file_read_link(filepath, 0);
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

int main(int argc, char* argv[]) {
	g_thread_init(NULL);

	gtk_init(&argc, &argv);

	gchar* filepath = find_module_filepath(argv[0]);
	gboolean res = puss_create(filepath);
	g_free(filepath);

	if( res ) {
		puss_run();
		puss_destroy();
	}

	return 0;
}

