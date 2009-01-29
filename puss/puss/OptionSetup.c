// OptionSetup.c
//

#include "OptionSetup.h"

#include "Puss.h"
#include "Utils.h"
#include "GlobalOptions.h"

typedef struct _SetupNode SetupNode;

struct _SetupNode {
	gchar*				id;
	gchar*				name;
	CreateSetupWidget	creator;
	gpointer			tag;
	GDestroyNotify		tag_destroy;
};

static void setup_node_free(SetupNode* node) {
	if( node ) {
		g_free(node->id);
		g_free(node->name);

		if( node->tag_destroy )
			node->tag_destroy(node->tag);

		g_free(node);
	}
}

static SetupNode*	puss_main_setup = 0;
static GHashTable*	puss_setup_panels = 0;

gboolean puss_option_setup_create() {
	puss_main_setup = g_new0(SetupNode, 1);
	puss_setup_panels = g_hash_table_new_full(g_str_hash, g_str_equal, 0, setup_node_free);
	if( puss_main_setup && puss_setup_panels ) {
		puss_main_setup->id = g_strdup( "puss" );
		puss_main_setup->name = g_strdup( _("puss") );
		puss_main_setup->creator = puss_create_global_options_setup_widget;
		return TRUE;
	}

	return FALSE;
}

void puss_option_setup_destroy() {
	g_hash_table_destroy(puss_setup_panels);

	setup_node_free(puss_main_setup);
	puss_main_setup = 0;
}

gboolean puss_option_setup_reg(const gchar* id, const gchar* name, CreateSetupWidget creator, gpointer tag, GDestroyNotify tag_destroy) {
	SetupNode* node;
	if( g_hash_table_lookup(puss_setup_panels, id) )
		return FALSE;

	node = g_new0(SetupNode, 1);
	node->id = g_strdup(id);
	node->name = g_strdup(name);
	node->creator = creator;
	node->tag = tag;
	node->tag_destroy = tag_destroy;

	g_hash_table_insert(puss_setup_panels, node->id, node);
	return TRUE;
}

void puss_option_setup_unreg(const gchar* id) {
	g_hash_table_remove(puss_setup_panels, id);
}

typedef struct {
	GtkListStore* store;
	const gchar* filter;
} FillListTag;

static gboolean do_fill_list(gchar* key, SetupNode* value, FillListTag* tag) {
	GtkTreeIter iter;

	gtk_list_store_append(tag->store, &iter);
	gtk_list_store_set( tag->store, &iter
		, 0, value->name
		, 1, value
		, -1 );

	return FALSE;
}

static void option_setup_fill_list(GtkListStore* store, const gchar* filter) {
	FillListTag tag = { store, filter };

	do_fill_list(puss_main_setup->id, puss_main_setup, &tag);

	g_hash_table_foreach(puss_setup_panels, (GHFunc)do_fill_list, &tag);
}

SIGNAL_CALLBACK void cb_option_setup_changed(GtkTreeView* tree_view, GtkContainer* frame) {
	GtkTreeIter iter;
	GtkTreePath* path = 0;
	GtkTreeModel* model;
	SetupNode* node = 0;
	GtkWidget* page;

	model = gtk_tree_view_get_model(tree_view);
	gtk_tree_view_get_cursor(tree_view, &path, 0);
	if( !gtk_tree_model_get_iter(model, &iter, path) )
		return;

	gtk_tree_model_get(model, &iter, 1, &node, -1);
	if( !node )
		return;

	page = gtk_bin_get_child(GTK_BIN(frame));
	if( page ) {
		gtk_container_remove(frame, page);
	}

	page = node->creator(node->tag);
	if( page ) {
		gtk_container_add(frame, page);
		gtk_widget_show_all(GTK_WIDGET(frame));
	}
}

void puss_option_setup_show_dialog(const gchar* filter) {
	gchar* filepath;
	GtkBuilder* builder;
	GtkWidget* dlg;
	GtkWidget* frame;
	GtkTreeView* view;
	GtkListStore* store;
	GError* err = 0;
	GtkTreePath* path;
	GtkTreeIter  iter;

	// create UI
	builder = gtk_builder_new();
	if( !builder )
		return;
	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	filepath = g_build_filename(puss_app->module_path, "res", "puss_option_dialog.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(puss) : build option setup ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(puss): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	dlg = GTK_WIDGET(gtk_builder_get_object(builder, "puss_option_dialog"));
	frame = GTK_WIDGET(gtk_builder_get_object(builder, "option_setup_panel"));
	view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "option_treeview"));
	store = GTK_LIST_STORE(gtk_builder_get_object(builder, "option_store"));
	gtk_builder_connect_signals(builder, frame);
	g_object_unref(G_OBJECT(builder));

	gtk_window_set_transient_for(GTK_WINDOW(dlg), puss_app->main_window);

	// fill data and show
	option_setup_fill_list(store, filter);

	if( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter) ) {
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
		if( path )
			gtk_tree_view_row_activated(view, path, 0);
	}

	gtk_widget_show_all(dlg);
	//gint res = 
	gtk_dialog_run(GTK_DIALOG(dlg));

	gtk_widget_destroy(dlg);
}

