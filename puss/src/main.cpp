// main.c

#include <libintl.h>
#include <gtk/gtk.h>

#include <glib/gi18n.h>


#include "Puss.h"

#define PACKAGE		"puss"
#define LOCALEDIR	"locale"

int main(int argc, char* argv[]) {
	g_thread_init(NULL);

	gtk_set_locale();
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gtk_init(&argc, &argv);

	gchar* filepath = g_find_program_in_path(argv[0]);
	if( !filepath ) {
		g_printerr(_("ERROR : can not find puss in $PATH\n"));
		return 1;
	}

	if( !g_path_is_absolute(filepath) ) {
		gchar* pwd = g_get_current_dir();
		gchar* prj =  g_build_filename(pwd, filepath, NULL);
		g_free(pwd);
		g_free(filepath);
		filepath = prj;
	}

	if( !g_file_test(filepath, G_FILE_TEST_EXISTS) ) {
		g_printerr(_("ERROR : can not find puss directory!\n"));
		g_printerr(_("        can not use indirect search path in $PATH!\n"));
		g_free(filepath);
		return 2;
	}

	Puss* app = puss_create(filepath);
	g_free(filepath);

	if( !app ) {
		g_printerr(_("ERROR : create application failed!\n"));
		return 3;
	}

	puss_run(app);

	puss_destroy(app);

	return 0;
}

