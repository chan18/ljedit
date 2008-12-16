// Puss.c

#include "Puss.h"

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

const gchar* puss_get_locale_path() {
	return puss_app->locale_path;
}

GtkBuilder* puss_get_ui_builder() {
	return puss_app->builder;
}

void init_puss_c_api(Puss* api) {
	// app
	api->get_module_path = &puss_get_module_path;
	api->get_locale_path = &puss_get_locale_path;

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

gboolean puss_load_ui_files() {
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

gboolean puss_main_ui_create() {
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
		&& puss_app->statusbar ) )
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

typedef struct _PageNode PageNode;

struct _PageNode {
	const gchar*	id;
	GtkWidget*		page;
	GtkWidget*		tab;
	GtkNotebook*	parent;

	PageNode*		next;
};

void puss_pop_all_pages(const gchar* nb_id, PageNode** pages) {
	gint i;
	gint count;
	PageNode* node;
	GtkNotebook* nb	= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, nb_id));
	if( !nb )
		return;

	count = gtk_notebook_get_n_pages(nb);
	for( i=0; i<count; ++i ) {
		node = g_new0(PageNode, 1);
		node->page = gtk_notebook_get_nth_page(nb, 0);
		node->tab  = gtk_notebook_get_tab_label(nb, node->page);
		g_object_ref(node->page);
		g_object_ref(node->tab);
		if( GTK_IS_LABEL(node->tab) )
			node->id = gtk_label_get_text(GTK_LABEL(node->tab));
		node->parent = nb;
		node->next = *pages;
		*pages = node;

		gtk_notebook_remove_page(nb, 0);
	}
}

void puss_nb_pages_set_order(GKeyFile* keyfile, const gchar* nb_id, PageNode** pages) {
	gchar** ids;
	gsize len = 0;
	gsize i;
	PageNode* p;
	PageNode* last;
	GtkNotebook* nb	= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, nb_id));
	if( !nb )
		return;

	ids= g_key_file_get_string_list(keyfile, "puss", nb_id, &len, 0);
	for(i=len; i>0; --i ) {
		last = 0;
		for( p = *pages; p; p=p->next ) {
			if( g_str_equal(p->id, ids[i-1]) ) {
				p->parent = nb;
				if( last )
					last->next = p->next;

				if( p!=*pages ) {
					p->next = *pages;
					*pages = p;
				}
				break;
			}

			last = p;
		}
	}
	g_strfreev(ids);
}

void puss_push_all_pages(PageNode* pages) {
	PageNode* p;

	while( pages ) {
		p = pages;
		pages = p->next;

		gtk_notebook_append_page(p->parent, p->page, p->tab);
		gtk_notebook_set_tab_reorderable(p->parent, p->page, TRUE);
		gtk_notebook_set_tab_detachable(p->parent, p->page, TRUE);

		g_object_unref(p->tab);
		g_object_unref(p->page);
		g_free(p);
	}
}

void puss_pages_reorder_load() {
	GKeyFile* keyfile;
	gchar* filepath;
	PageNode* pages = 0;

	puss_pop_all_pages("left_panel", &pages);
	puss_pop_all_pages("right_panel", &pages);
	puss_pop_all_pages("bottom_panel", &pages);

	keyfile = g_key_file_new();
	filepath = g_build_filename(g_get_user_config_dir(), ".puss_session", NULL);
	if( g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_NONE, 0) ) {
		puss_nb_pages_set_order(keyfile, "left_panel", &pages);
		puss_nb_pages_set_order(keyfile, "right_panel", &pages);
		puss_nb_pages_set_order(keyfile, "bottom_panel", &pages);
	}
	g_free(filepath);
	g_key_file_free(keyfile);

	puss_push_all_pages(pages);

	if( gtk_notebook_get_n_pages(puss_app->left_panel) > 0 )
		gtk_notebook_set_current_page(puss_app->left_panel, 0);

	if( gtk_notebook_get_n_pages(puss_app->right_panel) > 0 )
		gtk_notebook_set_current_page(puss_app->right_panel, 0);

	if( gtk_notebook_get_n_pages(puss_app->bottom_panel) > 0 )
		gtk_notebook_set_current_page(puss_app->bottom_panel, 0);
}

void puss_nb_pages_get_order(GKeyFile* keyfile, const gchar* nb_id) {
	gint i;
	gint count;
	const gchar** ids;
	GtkWidget* page;
	GtkWidget* tab;
	GtkNotebook* nb	= GTK_NOTEBOOK(gtk_builder_get_object(puss_app->builder, nb_id));
	if( !nb )
		return;

	count = gtk_notebook_get_n_pages(nb);
	if( count==0 )
		return;

	ids = g_new0(const gchar*, count);
	for( i=0; i<count; ++i ) {
		page = gtk_notebook_get_nth_page(nb, i);
		tab  = gtk_notebook_get_tab_label(nb, page);
		if( GTK_IS_LABEL(tab) )
			ids[i] = gtk_label_get_text(GTK_LABEL(tab));
		else
			ids[i] = "_not_rec_page_";
	}
	g_key_file_set_string_list(keyfile, "puss", nb_id, ids, count);
	g_free(ids);
}

void puss_pages_reorder_save() {
	GError* err = 0;
	gsize length = 0;
	gchar* content;
	gchar* filepath;
	GKeyFile* keyfile = g_key_file_new();

	puss_nb_pages_get_order(keyfile, "left_panel");
	puss_nb_pages_get_order(keyfile, "right_panel");
	puss_nb_pages_get_order(keyfile, "bottom_panel");

	content = g_key_file_to_data(keyfile, &length, &err);
	if( !content ) {
		g_printerr("ERROR(g_key_file_to_data) : %s\n", err->message);
		return;
	}
	g_key_file_free(keyfile);

	filepath = g_build_filename(g_get_user_config_dir(), ".puss_session", NULL);
	if( !g_file_set_contents(filepath, content, length, &err) )
		g_printerr("ERROR(g_file_set_contents) : %s\n", err->message);
	g_free(content);
	g_free(filepath);

}

void puss_locale_init() {
	gtk_set_locale();

	bindtextdomain(TEXT_DOMAIN, puss_app->locale_path);
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");
	//textdomain(TEXT_DOMAIN);
}

gboolean puss_create(const gchar* filepath) {
	puss_app = g_new0(PussApp, 1);

	puss_app->module_path = g_path_get_dirname(filepath);
	puss_app->locale_path = g_build_filename(puss_app->module_path, "locale", NULL);
	puss_app->extends_path = g_build_filename(puss_app->module_path, "extends", NULL);
	puss_app->extends_map = g_hash_table_new(g_str_hash, g_str_equal);

	puss_locale_init();

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

	g_hash_table_destroy(puss_app->extends_map);
	g_free(puss_app->module_path);
	g_free(puss_app->locale_path);
	g_free(puss_app->extends_path);
	g_free(puss_app);
	puss_app = 0;
}

void cb_puss_main_window_destroy() {
	puss_pages_reorder_save();

	gtk_main_quit();
}

void puss_run() {
	puss_pages_reorder_load();

	g_signal_connect(puss_app->main_window, "destroy", G_CALLBACK(&cb_puss_main_window_destroy), 0);

	gtk_widget_show( GTK_WIDGET(puss_app->main_window) );

	gtk_main();
}

