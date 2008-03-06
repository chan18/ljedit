// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "IPuss.h"
#include "MiniLine.h"
#include "DocManager.h"
#include "ExtendEngine.h"
#include "Utils.h"

void init_puss_c_api(C_API* api) {
	// app

	// main window

	// doc & view
	api->doc_set_url = &puss_doc_set_url;
	api->doc_get_url = &puss_doc_get_url;

	api->doc_set_charset = &puss_doc_set_charset;
	api->doc_get_charset = &puss_doc_get_charset;

	api->doc_get_view_from_page = &puss_doc_get_view_from_page;
	api->doc_get_buffer_from_page = &puss_doc_get_buffer_from_page;

	// doc manager
	api->doc_get_view_from_page_num   = &puss_doc_get_view_from_page_num;
	api->doc_get_buffer_from_page_num = &puss_doc_get_buffer_from_page_num;

	api->doc_find_page_from_url = &puss_doc_find_page_from_url;

	api->doc_new = &puss_doc_new;
	api->doc_open = &puss_doc_open;
	api->doc_locate = &puss_doc_locate;
	api->doc_save_current = &puss_doc_save_current;
	api->doc_close_current = &puss_doc_close_current;
	api->doc_save_all = &puss_doc_save_all;
	api->doc_close_all = &puss_doc_close_all;

	// mini line
	api->mini_line_active = &puss_mini_line_active;
	api->mini_line_deactive = &puss_mini_line_deactive;

	// utils
	api->send_focus_change = &puss_send_focus_change;
	api->active_panel_page = &puss_active_panel_page;
}

Puss* puss_create(const char* filepath) {
	Puss* app = g_new0(Puss, 1);
	if( app ) {
		memset(app, 0, sizeof(Puss));

		app->module_path = g_path_get_dirname(filepath);
		app->api = g_new0(C_API, 1);
		g_assert(app->module_path && app->api);
		init_puss_c_api(app->api);

		app->builder = gtk_builder_new();
		gchar* ui_file = g_build_filename(app->module_path, "res", "puss_ui.xml", NULL);
		if( ui_file ) {
			GError* err = 0;
			gtk_builder_add_from_file(app->builder, ui_file, &err);
			if (err)
				g_printerr("ERROR: %s\n", err->message);
			else
				gtk_builder_connect_signals(app->builder, app);
			g_free(ui_file);
		}

		GtkWindow* main_window = puss_get_main_window(app);

		// set icon
		gchar* icon_file = g_build_filename(app->module_path, "res", "puss.png", NULL);
		gtk_window_set_icon_from_file(main_window, icon_file, NULL);
		g_free(icon_file);

		g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), 0);

		gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(app->builder, "main_vbox")));

		puss_mini_line_create(app);

		puss_extend_engine_create(app);
	}

	return app;
}

void puss_destroy(Puss* app) {
	if( app ) {
		puss_extend_engine_destroy(app);

		puss_mini_line_destroy(app);

		// for debug, see main_window->ref_count
		GObject* main_window = G_OBJECT(puss_get_main_window(app));

		g_object_unref(G_OBJECT(app->builder));

		g_free(app->module_path);
		g_free(app->api);
		g_free(app);
	}
}

void puss_run(Puss* app) {
	g_assert( app );

	gtk_widget_show( GTK_WIDGET(puss_get_main_window(app)) );

	//g_print(_("test locale\n"));
	gtk_main();
}

