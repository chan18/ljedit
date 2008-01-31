// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>


void puss_create(Puss* app) {
	puss_create_ui(app);
}

void puss_run(Puss* app) {
	gtk_widget_show( GTK_WIDGET(app->ui.main_window) );

	g_signal_connect(G_OBJECT(app->ui.main_window), "destroy", G_CALLBACK(gtk_main_quit), 0);
	gtk_window_set_title(GTK_WINDOW(app->ui.main_window), _("title"));

	g_print(_("test locale\n"));
	gtk_main();
}

