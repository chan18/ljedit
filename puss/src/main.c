// main.c

#include <glib/gi18n.h>
#include "MainWindow.h"

#define PACKAGE "puss"
#define LOCALEDIR "locale"

int main(int argc, char* argv[]) {
	GtkWidget* mainwin;

	gtk_set_locale();
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gtk_init(&argc, &argv);
	mainwin = ljedit_main_window_new();
	gtk_widget_show_all(mainwin);

	g_signal_connect(G_OBJECT(mainwin), "destroy", G_CALLBACK(gtk_main_quit), 0);
	gtk_window_set_title(GTK_WINDOW(mainwin), _("title"));

	g_print(_("title"));
	gtk_main();
}


