// Puss.c

#include "Puss.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "DocManager.h"
#include "DocSearch.h"
#include "ExtendManager.h"
#include "PluginManager.h"
#include "PosLocate.h"
#include "PanelOrder.h"
#include "OptionManager.h"
#include "OptionSetup.h"
#include "GlobalOptions.h"
#include "Utils.h"

PussApp* puss_app = 0;

static const gchar* puss_get_module_path() {
	return puss_app->module_path;
}

static const gchar* puss_get_locale_path() {
	return puss_app->locale_path;
}

static const gchar* puss_get_extends_path() {
	return puss_app->extends_path;
}

static const gchar* puss_get_plugins_path() {
	return puss_app->plugins_path;
}

static GtkBuilder* puss_get_ui_builder() {
	return puss_app->builder;
}

static gboolean puss_load_ui(const gchar* filename ) {
	GError* err = 0;
	gchar* filepath = g_build_filename(puss_app->module_path, "res", filename, NULL);
	if( !filepath ) {
		g_printerr("ERROR(puss) : build ui filepath failed!\n");
		return FALSE;
	}

	gtk_builder_add_from_file(puss_app->builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(puss): %s\n", err->message);
		g_error_free(err);
		return FALSE;
	}

	return TRUE;
}

static gboolean puss_load_ui_files() {
	puss_app->builder = gtk_builder_new();
	if( !puss_app->builder ) {
		g_printerr("ERROR(puss) : gtk_builder_new failed!\n");
		return FALSE;
	}
	gtk_builder_set_translation_domain(puss_app->builder, TEXT_DOMAIN);

	if( !( puss_load_ui("puss_ui_manager.xml")
		&& puss_load_ui("puss_main_window.xml") ) )
	{
		return FALSE;
	}

	gtk_builder_connect_signals(puss_app->builder, 0);

	return TRUE;
}

static gboolean puss_main_ui_create() {
	gchar* icon_file;

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
		&& puss_app->statusbar
		&& puss_find_dialog_init(puss_app->builder) ) )
	{
		return FALSE;
	}

	// set icon
	icon_file = g_build_filename(puss_app->module_path, "res", "puss.png", NULL);
	gtk_window_set_icon_from_file(puss_app->main_window, icon_file, 0);
	g_free(icon_file);

	gtk_widget_show_all( gtk_bin_get_child(GTK_BIN(puss_app->main_window)) );

	return TRUE;
}

static void puss_locale_init() {
	gtk_set_locale();

	bindtextdomain(TEXT_DOMAIN, puss_app->locale_path);
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");
}

static void cb_puss_main_window_destroy() {
	puss_panel_order_save();
	puss_plugin_manager_unload_all();

	gtk_main_quit();
}

static Puss	__api__ = {
	// app
	  puss_get_module_path
	, puss_get_locale_path
	, puss_get_extends_path
	, puss_get_plugins_path 

	// UI
	, puss_get_ui_builder
	, puss_panel_append
	, puss_panel_remove
	, puss_panel_get_pos

	// doc & view
	, puss_doc_set_url
	, puss_doc_get_url
	, puss_doc_set_charset
	, puss_doc_get_charset
	, puss_doc_get_view_from_page
	, puss_doc_get_buffer_from_page

	// doc manager
	, puss_doc_get_view_from_page_num
	, puss_doc_get_buffer_from_page_num
	, puss_doc_find_page_from_url
	, puss_doc_new
	, puss_doc_open
	, puss_doc_open_locate
	, puss_doc_locate
	, puss_doc_save_current
	, puss_doc_close_current
	, puss_doc_save_all
	, puss_doc_close_all

	// utils
	, puss_send_focus_change
	, puss_active_panel_page
	, puss_load_file
	, puss_format_filename

	// option manager
	, puss_option_manager_option_reg
	, puss_option_manager_option_find
	, puss_option_manager_option_set
	, puss_option_manager_monitor_reg
	, puss_option_manager_monitor_unreg

	// option setup
	, puss_option_setup_reg
	, puss_option_setup_unreg

	// extend manager
	, puss_extend_manager_query

	// plugin manager
	, puss_plugin_engine_regist

	// search utils
	, puss_find_and_locate_text
};

gboolean puss_create(const gchar* filepath) {
	puss_app = g_new0(PussApp, 1);
	puss_app->api = &__api__;

	puss_app->module_path = g_path_get_dirname(filepath);
	puss_app->locale_path = g_build_filename(puss_app->module_path, "locale", NULL);
	puss_app->extends_path = g_build_filename(puss_app->module_path, "extends", NULL);
	puss_app->plugins_path = g_build_filename(puss_app->module_path, "plugins", NULL);
	puss_app->extends_map = g_hash_table_new(g_str_hash, g_str_equal);

	puss_locale_init();

	if( !puss_option_manager_create() ) {
		g_printerr("ERROR(puss) : create option manager failed!\n");
		return FALSE;
	}

	puss_reg_global_options();

	if( !( puss_utils_create()
		&& puss_load_ui_files()
		&& puss_main_ui_create()
		&& puss_doc_manager_create()
		&& puss_pos_locate_create()
		&& puss_option_setup_create()
		&& puss_plugin_manager_create()
		&& puss_extend_manager_create() ) )
	{
		return FALSE;
	}

	puss_panel_order_load();
	return TRUE;
}

void puss_destroy() {
	puss_extend_manager_destroy();
	puss_plugin_manager_destroy();
	puss_pos_locate_destroy();
	puss_doc_manager_destroy();
	puss_utils_destroy();
	puss_option_setup_destroy();

	puss_option_manager_destroy();

	g_object_unref(G_OBJECT(puss_app->builder));

	g_hash_table_destroy(puss_app->extends_map);
	g_free(puss_app->plugins_path);
	g_free(puss_app->extends_path);
	g_free(puss_app->locale_path);
	g_free(puss_app->module_path);
	g_free(puss_app);
	puss_app = 0;
}

void puss_run() {
	g_signal_connect(puss_app->main_window, "destroy", G_CALLBACK(&cb_puss_main_window_destroy), 0);

	puss_plugin_manager_load_all();

	gtk_notebook_set_current_page(puss_app->left_panel, 0);
	gtk_notebook_set_current_page(puss_app->right_panel, 0);
	gtk_notebook_set_current_page(puss_app->bottom_panel, 0);

	gtk_widget_show( GTK_WIDGET(puss_app->main_window) );
	gdk_window_show( gtk_widget_get_window(GTK_WIDGET(puss_app->main_window)) );

	gtk_main();
}

