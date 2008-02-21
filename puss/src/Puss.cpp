// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>

#include "IPuss.h"
#include "MainWindow.h"
#include "MiniLine.h"
#include "PluginEngine.h"

Puss* puss_create() {
	Puss* app = (Puss*)g_malloc(sizeof(Puss));
	if( app ) {
		puss_main_window_create(app);
		puss_mini_line_create(app);
		puss_plugin_engine_create(app);
	}
	return app;
}

void puss_destroy(Puss* app) {
	g_assert( app );

	puss_plugin_engine_destroy(app);
	puss_mini_line_destroy(app);
	puss_main_window_destroy(app);

	g_free(app);
}

void puss_run(Puss* app) {
	g_assert( app );

	gtk_widget_show( GTK_WIDGET(app->main_window->window) );

	g_signal_connect(G_OBJECT(app->main_window->window), "destroy", G_CALLBACK(gtk_main_quit), 0);
	gtk_window_set_title(GTK_WINDOW(app->main_window->window), _("Puss - c/c++ source editor"));

	//g_print(_("test locale\n"));
	gtk_main();
}

