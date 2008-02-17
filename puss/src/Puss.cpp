// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>


void puss_create(Puss* app) {
	puss_main_window_create(app);
	puss_mini_line_create(app);
}

void puss_run(Puss* app) {
	gtk_widget_show( GTK_WIDGET(app->main_window.window) );

	g_signal_connect(G_OBJECT(app->main_window.window), "destroy", G_CALLBACK(gtk_main_quit), 0);
	gtk_window_set_title(GTK_WINDOW(app->main_window.window), _("title"));

	g_print(_("test locale\n"));
	gtk_main();
}

