// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "IPuss.h"
#include "MainWindow.h"
#include "MiniLine.h"
#include "DocManager.h"
#include "Utils.h"
#include "ExtendEngine.h"

C_API* create_puss_c_api() {
	C_API* api = (C_API*)g_malloc(sizeof(C_API));
	memset(api, 0, sizeof(C_API));

	// app

	// main window

	// doc & view
	api->doc_set_url = &puss_doc_set_url;
	api->doc_get_url = &puss_doc_get_url;

	api->doc_set_charset = &puss_doc_set_charset;
	api->doc_get_charset = &puss_doc_get_charset;

	api->doc_replace_all = &puss_doc_replace_all;

	api->doc_get_view_from_page = &puss_doc_get_view_from_page;
	api->doc_get_buffer_from_page = &puss_doc_get_buffer_from_page;

	// doc manager
	api->doc_get_label_from_page_num  = &puss_doc_get_label_from_page_num;
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

	return api;
}

Puss* puss_create(const char* filepath) {
	Puss* app = (Puss*)g_malloc(sizeof(Puss));
	memset(app, 0, sizeof(Puss));

	app->module_path = g_path_get_dirname(filepath);
	g_assert(app->module_path);

	app->api = create_puss_c_api();
	g_assert(app->api);

	if( app ) {
		puss_main_window_create(app);
		puss_mini_line_create(app);
		puss_extend_engine_create(app);
	}
	return app;
}

void puss_destroy(Puss* app) {
	g_assert( app );

	puss_extend_engine_destroy(app);
	puss_mini_line_destroy(app);
	puss_main_window_destroy(app);

	g_free(app->module_path);
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

