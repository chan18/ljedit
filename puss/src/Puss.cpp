// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>


void Puss::create() {
	main_window_.create("");
}

void Puss::run() {
	gtk_widget_show( GTK_WIDGET(main_window_.window) );

	g_signal_connect(G_OBJECT(main_window_.window), "destroy", G_CALLBACK(gtk_main_quit), 0);
	gtk_window_set_title(GTK_WINDOW(main_window_.window), _("title"));

	g_print(_("title"));
	gtk_main();
}

