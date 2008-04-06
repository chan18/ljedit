// Puss.cpp

#include "Puss.h"

#include <glib/gi18n.h>
#include <memory.h>
#include <stdlib.h>

#include "MiniLine.h"
#include "DocManager.h"
#include "ExtendEngine.h"
#include "PosLocate.h"
#include "OptionManager.h"
#include "GlobalOptions.h"
#include "Utils.h"

PussApp* puss_app = 0;

const gchar* puss_get_module_path() {
	return puss_app->module_path;
}

GtkBuilder* puss_get_ui_builder() {
	return puss_app->builder;
}

void init_puss_c_api(Puss* api) {
	// app
	api->get_module_path = &puss_get_module_path;

	// UI
	api->get_ui_builder = &puss_get_ui_builder;

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
	api->load_file = &puss_load_file;
	api->format_filename = &puss_format_filename;

	// option manager
	api->option_manager_find = &puss_option_manager_find;
	api->option_manager_option_reg  = &puss_option_manager_option_reg;
	api->option_manager_monitor_reg = &puss_option_manager_monitor_reg;
}

gboolean puss_load_ui(const gchar* filename ) {
	gchar* filepath = g_build_filename(puss_app->module_path, "res", filename, NULL);
	if( !filepath ) {
		g_printerr("ERROR(puss) : build ui filepath failed!\n");
		return FALSE;
	}

	GError* err = 0;
	gtk_builder_add_from_file(puss_app->builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(puss): %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	return TRUE;
}

gboolean puss_load_ui_files() {
	puss_app->builder = gtk_builder_new();
	if( !puss_app->builder ) {
		g_printerr("ERROR(puss) : gtk_builder_new failed!\n");
		return FALSE;
	};

	if( !( puss_load_ui("puss_ui_manager.xml")
		&& puss_load_ui("puss_main_window.xml")
		&& puss_load_ui("puss_mini_window.xml") ) )
	{
		return FALSE;
	}

	gtk_builder_connect_signals(puss_app->builder, 0);

	return TRUE;
}

gboolean puss_main_ui_create() {
	puss_app->main_window	= GTK_WINDOW(gtk_builder_get_object(puss_app->builder, "main_window"));
	puss_app->ui_manager	= GTK_UI_MANAGER(gtk_builder_get_object(puss_app->builder, "main_ui_manager"));
	puss_app->doc_panel		= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, "doc_panel"));
	puss_app->left_panel	= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, "left_panel"));
	puss_app->right_panel	= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, "right_panel"));
	puss_app->bottom_panel	= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, "bottom_panel"));
	puss_app->statusbar		= GTK_STATUSBAR(gtk_builder_get_object(puss_app->builder, "statusbar"));

	if( !( puss_app->main_window
		&& puss_app->ui_manager
		&& puss_app->doc_panel
		&& puss_app->left_panel
		&& puss_app->right_panel
		&& puss_app->bottom_panel
		&& puss_app->statusbar ) )
	{
		return FALSE;
	}

	// set icon
	gchar* icon_file = g_build_filename(puss_app->module_path, "res", "puss.png", NULL);
	gtk_window_set_icon_from_file(puss_app->main_window, icon_file, 0);
	g_free(icon_file);

	g_signal_connect(puss_app->main_window, "destroy", G_CALLBACK(&gtk_main_quit), 0);

	gtk_widget_show_all( gtk_bin_get_child(GTK_BIN(puss_app->main_window)) );

	return TRUE;
}

gboolean puss_create(const char* filepath) {
	puss_app = g_new0(PussApp, 1);
	if( !puss_app ) {
		g_printerr("ERROR(puss) : new puss app failed!\n");
		return FALSE;
	}

	puss_app->module_path = g_path_get_dirname(filepath);
	init_puss_c_api((Puss*)puss_app);

	if( !puss_option_manager_create() ) {
		g_printerr("ERROR(puss) : create option manager failed!\n");
		return FALSE;
	}

	puss_reg_global_options();

	return puss_utils_create()
		&& puss_doc_manager_create()
		&& puss_load_ui_files()
		&& puss_main_ui_create()
		&& puss_mini_line_create()
		&& puss_pos_locate_create()
		&& puss_extend_engine_create();
}

void puss_destroy() {
	puss_extend_engine_destroy();
	puss_pos_locate_destroy();
	puss_mini_line_destroy();
	puss_doc_manager_destroy();
	puss_utils_destroy();

	puss_option_manager_destroy();

	g_object_unref(G_OBJECT(puss_app->builder));

	g_free(puss_app->module_path);
	g_free(puss_app);
	puss_app = 0;
}

void puss_run() {
	if( gtk_notebook_get_n_pages(puss_app->left_panel) > 0 )
		gtk_notebook_set_current_page(puss_app->left_panel, 0);

	if( gtk_notebook_get_n_pages(puss_app->right_panel) > 0 )
		gtk_notebook_set_current_page(puss_app->right_panel, 0);

	if( gtk_notebook_get_n_pages(puss_app->bottom_panel) > 0 )
		gtk_notebook_set_current_page(puss_app->bottom_panel, 0);

	gtk_widget_show( GTK_WIDGET(puss_app->main_window) );

	//g_print(_("test locale\n"));
	gtk_main();
}

