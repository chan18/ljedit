// Puss.c

#include "Puss.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "DocManager.h"
#include "ExtendManager.h"
#include "PluginManager.h"
#include "PosLocate.h"
#include "OptionManager.h"
#include "GlobalOptions.h"
#include "Utils.h"

PussApp* puss_app = 0;

#define PUSS_PANEL_SORT_ID "__PUSS_PANEL_SORT_ID__"

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

typedef struct _PosNode PosNode;

struct _PosNode {
	PosNode*	prev;
	PosNode*	next;
	GtkWidget*	panel;
	gchar		id[0];
};

static PosNode* puss_left_panel_order_list = 0;
static PosNode* puss_right_panel_order_list = 0;
static PosNode* puss_bottom_panel_order_list = 0;

static PosNode* panel_pos_new(const gchar* id) {
	size_t len = strlen(id);
	PosNode* pos = g_malloc0(sizeof(PosNode) + (gsize)len + 1);
	memcpy(pos->id, id, len + 1);
	return pos;
}

static PosNode* option_order_list_parse(const gchar* option_value) {
	PosNode* ret = 0;
	PosNode* ps = 0;
	PosNode* pe = 0;

	if( option_value ) {
		gchar** p;
		gchar** items = g_strsplit_set(option_value, ",; \t\r\n", 0);
		for( p=items; *p; ++p ) {
			if( *p[0]=='\0' )
				continue;
			pe = panel_pos_new(*p);
			if( ps )
				ps->next = pe;
			pe->prev = ps;
			ps = pe;
			if( !ret )
				ret = ps;
		}
		g_strfreev(items);
	}

	return ret;
}

#define MAX_ORDER_LEN 1024

static gchar* option_order_list_build(PosNode* order_list) {
	guint i;
	PosNode* ps;
	gchar* arr[MAX_ORDER_LEN+1];
	gchar** pd;

	ps=order_list;
	pd = arr;

	for( i=0; i<MAX_ORDER_LEN && ps; ++i ) {
		*pd = ps->id;
		++pd;
		ps = ps->next;
	}
	*pd = 0;

	return g_strjoinv(" ", arr);
}

/*
static void puss_pop_all_pages(const gchar* nb_id, PageNode** pages) {
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

static void puss_nb_pages_set_order(GKeyFile* keyfile, const gchar* nb_id, PageNode** pages) {
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

static void puss_push_all_pages(PageNode* pages) {
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
*/

static void reorder_panel(GtkNotebook* notebook, guint page_num, PosNode* pos, PosNode** order_list) {
	PosNode* p;
	guint i;

	p = 0;
	for( i=page_num; !p && i>0; --i )
		p = (PosNode*)g_object_get_data( G_OBJECT(gtk_notebook_get_nth_page(notebook, i-1)), PUSS_PANEL_SORT_ID );

	if( p ) {
		pos->prev = p;
		pos->next = p->next;
		if( p->next )
			p->next->prev = pos;
		p->next = pos;
	} else {
		pos->prev = 0;
		pos->next = *order_list;
		if( pos->next )
			pos->next->prev = pos;
		*order_list = pos;
	}
}

static void cb_panel_reordered(GtkNotebook* notebook, GtkWidget* child, guint page_num, PosNode** order_list) {
	PosNode* pos;

	pos = (PosNode*)g_object_get_data(G_OBJECT(child), PUSS_PANEL_SORT_ID);
	if( !pos )
		return;

	if( pos->prev )
		pos->prev->next = pos->next;
	else
		*order_list = pos->next;

	if( pos->next )
		pos->next->prev = pos->prev;

	reorder_panel(notebook, page_num, pos, order_list);
}

static void cb_panel_added(GtkNotebook* notebook, GtkWidget* child, guint page_num, PosNode** order_list) {
	PosNode* pos = (PosNode*)g_object_get_data(G_OBJECT(child), PUSS_PANEL_SORT_ID);
	if( !pos )
		return;

	pos->panel = child;
	if( pos->prev ) {
		pos->prev->next = pos->next;
	} else {
		if( pos==puss_left_panel_order_list )
			puss_left_panel_order_list = pos->next;
		else if( pos==puss_right_panel_order_list )
			puss_right_panel_order_list = pos->next;
		else if( pos==puss_bottom_panel_order_list )
			puss_bottom_panel_order_list = pos->next;
	}

	if( pos->next )
		pos->next->prev = pos->prev;

	reorder_panel(notebook, page_num, pos, order_list);
}

static void cb_panel_removed(GtkNotebook* notebook, GtkWidget* child, guint page_num, PosNode** order_list) {
	PosNode* pos = (PosNode*)g_object_get_data(G_OBJECT(child), PUSS_PANEL_SORT_ID);
	if( !pos )
		return;
	pos->panel = 0;
}

static void puss_panel_order_load() {
	const Option* option;

	option = puss_option_manager_option_reg("puss", "left_panels_order", "");
	puss_left_panel_order_list = option_order_list_parse(option->value);
	g_signal_connect(puss_app->left_panel, "page-reordered", G_CALLBACK(cb_panel_reordered), &puss_left_panel_order_list);
	g_signal_connect(puss_app->left_panel, "page-added", G_CALLBACK(cb_panel_added), &puss_left_panel_order_list);
	g_signal_connect(puss_app->left_panel, "page-removed", G_CALLBACK(cb_panel_removed), &puss_left_panel_order_list);

	option = puss_option_manager_option_reg("puss", "right_panels_order", "");
	puss_right_panel_order_list = option_order_list_parse(option->value);
	g_signal_connect(puss_app->right_panel, "page-reordered", G_CALLBACK(cb_panel_reordered), &puss_right_panel_order_list);
	g_signal_connect(puss_app->right_panel, "page-added", G_CALLBACK(cb_panel_added), &puss_right_panel_order_list);
	g_signal_connect(puss_app->right_panel, "page-removed", G_CALLBACK(cb_panel_removed), &puss_right_panel_order_list);

	option = puss_option_manager_option_reg("puss", "bottom_panels_order", "");
	puss_bottom_panel_order_list = option_order_list_parse(option->value);
	g_signal_connect(puss_app->bottom_panel, "page-reordered", G_CALLBACK(cb_panel_reordered), &puss_bottom_panel_order_list);
	g_signal_connect(puss_app->bottom_panel, "page-added", G_CALLBACK(cb_panel_added), &puss_bottom_panel_order_list);
	g_signal_connect(puss_app->bottom_panel, "page-removed", G_CALLBACK(cb_panel_removed), &puss_bottom_panel_order_list);
}

static void puss_panel_order_save() {
	const Option* option;
	gchar* option_value;

	option = puss_option_manager_option_find("puss", "left_panels_order");
	option_value = option_order_list_build(puss_left_panel_order_list);
	puss_option_manager_option_set(option, option_value);
	g_free(option_value);

	option = puss_option_manager_option_find("puss", "right_panels_order");
	option_value = option_order_list_build(puss_right_panel_order_list);
	puss_option_manager_option_set(option, option_value);
	g_free(option_value);

	option = puss_option_manager_option_find("puss", "bottom_panels_order");
	option_value = option_order_list_build(puss_bottom_panel_order_list);
	puss_option_manager_option_set(option, option_value);
	g_free(option_value);
}

static gboolean puss_panel_get_pos(GtkWidget* panel, GtkNotebook** parent, gint* page_num) {
	gint i;
	gint count;
	GtkWidget* page;
	GtkNotebook* nb;
	
	if( panel ) {
		nb = GTK_NOTEBOOK( gtk_widget_get_parent(panel) );
		count = gtk_notebook_get_n_pages(nb);
		for( i=0; i<count; ++i ) {
			page = gtk_notebook_get_nth_page(nb, i);
			if( page==panel ) {
				if( parent )
					*parent = nb;
				if( page_num )
					*page_num = i;
				return TRUE;
			}
		}
	}

	return FALSE;
}

static void puss_panel_append(GtkWidget* panel, GtkWidget* tab_label, const gchar* id, PanelPosition default_pos) {
	GtkNotebook* parent = 0;
	PosNode** order_list = 0;
	PosNode* p = 0;
	PosNode* pos = 0;
	gint page_pos = 0;
	gint page_num;

	if( !parent ) {
		for( pos=puss_left_panel_order_list; pos; pos=pos->next) {
			if( g_str_equal(pos->id, id) ) {
				parent = puss_app->left_panel;
				order_list = &puss_left_panel_order_list;
				break;
			}
		}
	}

	if( !parent ) {
		for( pos=puss_right_panel_order_list; pos; pos=pos->next) {
			if( g_str_equal(pos->id, id) ) {
				parent = puss_app->right_panel;
				order_list = &puss_right_panel_order_list;
					break;
			}
		}
	}

	if( !parent ) {
		for( pos=puss_bottom_panel_order_list; pos; pos=pos->next) {
			if( g_str_equal(pos->id, id) ) {
				parent = puss_app->bottom_panel;
				order_list = &puss_bottom_panel_order_list;
					break;
			}
		}
	}

	if( !parent ) {
		switch( default_pos ) {
		case PUSS_PANEL_POS_HIDE:
			// TODO : use hidden notebook
		case PUSS_PANEL_POS_LEFT:
			parent = puss_app->left_panel;
			order_list = &puss_left_panel_order_list;
			break;
		case PUSS_PANEL_POS_RIGHT:
			parent = puss_app->right_panel;
			order_list = &puss_right_panel_order_list;
			break;
		case PUSS_PANEL_POS_BOTTOM:
		default:
			parent = puss_app->bottom_panel;
			order_list = &puss_bottom_panel_order_list;
			break;
		}
		pos = panel_pos_new(id);
		if( *order_list ) {
			for( p=*order_list; p->next; p=p->next );
			p->next = pos;
			pos->prev = p;
		} else {
			*order_list = pos;
		}
	}

	g_object_set_data(G_OBJECT(panel), PUSS_PANEL_SORT_ID, pos);
	pos->panel = panel;

	for( p=pos->prev; p; p=p->prev ) {
		if( p->panel ) {
			page_pos = gtk_notebook_page_num(parent, p->panel) + 1;
			break;
		}
	}

	page_num = gtk_notebook_get_n_pages(parent);
	g_signal_handlers_block_by_func(parent, &cb_panel_added, order_list);
	if( page_pos >= page_num )
		gtk_notebook_append_page(parent, panel, tab_label);
	else
		gtk_notebook_insert_page(parent, panel, tab_label, page_pos);
	g_signal_handlers_unblock_by_func(parent, &cb_panel_added, order_list);

	gtk_notebook_set_tab_reorderable(parent, panel, TRUE);
	gtk_notebook_set_tab_detachable(parent, panel, TRUE);
}

static void puss_panel_remove(GtkWidget* panel) {
	gint page_num;
	GtkNotebook* parent;
	PosNode* pos;

	pos = (PosNode*)g_object_get_data(G_OBJECT(panel), PUSS_PANEL_SORT_ID);
	if( pos )
		pos->panel = 0;

	if( puss_panel_get_pos(panel, &parent, &page_num) )
		gtk_notebook_remove_page(parent, page_num);
}


static void init_puss_c_api(Puss* api) {
	// app
	api->get_module_path = &puss_get_module_path;
	api->get_locale_path = &puss_get_locale_path;
	api->get_extends_path = &puss_get_extends_path;
	api->get_plugins_path = &puss_get_plugins_path; 

	// UI
	api->get_ui_builder = &puss_get_ui_builder;
	api->panel_append = &puss_panel_append;
	api->panel_remove = &puss_panel_remove;
	api->panel_get_pos = &puss_panel_get_pos;

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

	// utils
	api->send_focus_change = &puss_send_focus_change;
	api->active_panel_page = &puss_active_panel_page;
	api->load_file = &puss_load_file;
	api->format_filename = &puss_format_filename;

	// option manager
	api->option_reg = &puss_option_manager_option_reg;
	api->option_find = &puss_option_manager_option_find;
	api->option_set = &puss_option_manager_option_set;

	api->option_monitor_reg = &puss_option_manager_monitor_reg;
	api->option_monitor_unreg = &puss_option_manager_monitor_unreg;

	// extend manager
	api->extend_query = &puss_extend_manager_query;

	// plugin manager
	api->plugin_engine_regist = &puss_plugin_engine_regist;
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

gboolean puss_create(const gchar* filepath) {
	puss_app = g_new0(PussApp, 1);

	puss_app->module_path = g_path_get_dirname(filepath);
	puss_app->locale_path = g_build_filename(puss_app->module_path, "locale", NULL);
	puss_app->extends_path = g_build_filename(puss_app->module_path, "extends", NULL);
	puss_app->plugins_path = g_build_filename(puss_app->module_path, "plugins", NULL);
	puss_app->extends_map = g_hash_table_new(g_str_hash, g_str_equal);

	puss_locale_init();

	init_puss_c_api((Puss*)puss_app);

	if( !puss_option_manager_create() ) {
		g_printerr("ERROR(puss) : create option manager failed!\n");
		return FALSE;
	}

	puss_reg_global_options();

	if( !( puss_utils_create()
		&& puss_doc_manager_create()
		&& puss_load_ui_files()
		&& puss_main_ui_create()
		&& puss_pos_locate_create()
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

	gtk_main();
}

