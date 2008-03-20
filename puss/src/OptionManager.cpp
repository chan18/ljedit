// OptionManager.cpp
//

#include "OptionManager.h"

#include "Puss.h"

struct OptionNotifer {
	OptionNotifer*		next;
	OptionChanged		fun;
	gpointer			tag;
};

struct OptionNode {
	const gchar*	group;
	const gchar*	key;

	gchar*			current_value;
	gchar*			default_value;
	OptionSetter	setter_fun;
	gpointer		setter_tag;

	OptionNotifer*	notifer_list;
};

void option_node_free(OptionNode* node) {
	g_free(node->current_value);
	g_free(node->default_value);

	while( node->notifer_list ) {
		OptionNotifer* p = node->notifer_list;
		node->notifer_list = p->next;
		g_free(p);
	}

	g_free(node);
}

struct OptionManager {
	gchar*		filepath;
	GKeyFile*	keyfile;
	gboolean	modified;

	GHashTable*	option_groups;	// group, GHashTable(key, OptionNode)
};

gboolean puss_option_manager_create() {
	g_assert( !puss_app->option_manager );

	puss_app->option_manager = g_new0(OptionManager, 1);
	if( !puss_app->option_manager ) {
		g_printerr("ERROR(option_manager) : new option manager failed!\n");
		return FALSE;
	}

	OptionManager* self = puss_app->option_manager;
	self->filepath = g_build_filename(g_get_user_config_dir(), ".puss_config", NULL);
	if( !self->filepath ) {
		g_printerr("ERROR(option_manager) : build filename failed!\n");
		return FALSE;
	}

	self->keyfile = g_key_file_new();
	if( !self->keyfile  ) {
		g_printerr("ERROR(option_manager) : new key file failed!\n");
		return FALSE;
	}

	self->option_groups = (GHashTable*)g_hash_table_new_full( &g_str_hash, &g_str_equal, &g_free, GDestroyNotify(&g_hash_table_destroy) );
	if( !self->option_groups ) {
		g_printerr("ERROR(option_manager) : new hash table failed!\n");
		return FALSE;
	}

	// now ignore error, TODO :
	// 
	g_key_file_load_from_file(self->keyfile, self->filepath, G_KEY_FILE_KEEP_COMMENTS, 0);

	return TRUE;
}

void puss_option_manager_destroy() {
	OptionManager* self = puss_app->option_manager;
	puss_app->option_manager = 0;

	if( self ) {
		g_key_file_free(self->keyfile);
		g_free(self->filepath);
		g_hash_table_destroy(self->option_groups);

		g_free(self);
	}
}

gboolean option_manager_check_option(OptionNode* option) {
	OptionManager* self = puss_app->option_manager;

	gchar* value = g_key_file_get_value(self->keyfile, option->group, option->key, 0);
	if( value==0 && option->current_value==0 )
		return FALSE;

	if( value && option->current_value && g_str_equal(value, option->current_value) )
		return FALSE;

	for( OptionNotifer* p = option->notifer_list; p; p = p->next ) {
		g_assert( p->fun );
		p->fun(option->group, option->key, value, option->current_value, p->tag);
	}

	g_free(option->current_value);
	option->current_value = value;
	self->modified= TRUE;
	return TRUE;
}

void option_manager_save() {
	OptionManager* self = puss_app->option_manager;

	GError* err = 0;
	gsize length = 0;
	gchar* content = g_key_file_to_data(self->keyfile, &length, &err);
	if( !content ) {
		g_printerr("ERROR(g_key_file_to_data) : %s\n", err->message);
		return;
	}

	if( !g_file_set_contents(self->filepath, content, length, &err) )
		g_printerr("ERROR(g_file_set_contents) : %s\n", err->message);

	g_free(content);
}

gboolean puss_option_manager_option_reg(const gchar* group, const gchar* key, const gchar* default_value, OptionSetter fun, gpointer tag) {
	OptionManager* self = puss_app->option_manager;

	gchar* ptr_group = g_strdup(group);
	gchar* ptr_key = g_strdup(key);

	GHashTable* option_group = (GHashTable*)g_hash_table_lookup(self->option_groups, group);
	if( !option_group ) {
		option_group = (GHashTable*)g_hash_table_new_full( &g_str_hash, &g_str_equal, &g_free, GDestroyNotify(&option_node_free) );
		if( !option_group )
			return FALSE;
		g_hash_table_insert(self->option_groups, ptr_group, option_group);
	}

	OptionNode* node = (OptionNode*)g_hash_table_lookup(option_group, key);
	if( node )
		return FALSE;

	node = g_new0(OptionNode, 1);
	if( node ) {
		node->group = ptr_group;
		node->key = ptr_key;

		node->current_value = g_key_file_get_value(self->keyfile, group, key, 0);
		if( default_value ) {
			node->default_value = g_strdup(default_value);
			if( !node->current_value )
				node->current_value = g_strdup(default_value);
		}

		node->setter_fun = fun;
		node->setter_tag = tag;
		node->notifer_list = 0;

		g_hash_table_insert(option_group, ptr_key, node);
		return TRUE;
	}

	return TRUE;
}

OptionNode* option_manager_find(const gchar* group, const gchar* key) {
	OptionManager* self = puss_app->option_manager;
	GHashTable* option_group = (GHashTable*)g_hash_table_lookup(self->option_groups, group);
	return option_group ? (OptionNode*)g_hash_table_lookup(option_group, key) : 0;
}

gboolean puss_option_manager_monitor_reg(const gchar* group, const gchar* key, OptionChanged fun, gpointer tag ) {
	OptionNode* node = option_manager_find(group, key);
	if( !node )
		return FALSE;

	OptionNotifer* notifer = g_new0(OptionNotifer, 1);
	if( !notifer )
		return FALSE;

	notifer->next = node->notifer_list;
	notifer->fun = fun;
	notifer->tag = tag;
	node->notifer_list = notifer;
	return TRUE;
}

gboolean puss_default_option_setter(GtkWindow* parent, GKeyFile* options, const gchar* group, const gchar* key, gpointer tag) {
	OptionNode* option = option_manager_find(group, key);
	if( !option )
		return FALSE;

	GtkWidget* dlg = gtk_dialog_new_with_buttons( "option setting..."
		, parent
		, GTK_DIALOG_MODAL
		, GTK_STOCK_OK, GTK_RESPONSE_OK
		, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
		, NULL );

	GtkWidget* label = gtk_label_new(key);
	GtkWidget* entry = gtk_entry_new();
	if( option->current_value )
		gtk_entry_set_text(GTK_ENTRY(entry), option->current_value);

	GtkWidget* hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

	GtkWidget* frame = gtk_frame_new(group);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show_all(frame);

	gint res = gtk_dialog_run(GTK_DIALOG(dlg));

	if( res==GTK_RESPONSE_OK )
	{
		const gchar* value = gtk_entry_get_text(GTK_ENTRY(entry));
		if( value )
			g_key_file_set_value(options, group, key, value);
		else
			res = GTK_RESPONSE_CANCEL;
	}

	gtk_widget_destroy(dlg);

	return res==GTK_RESPONSE_OK;
}

struct ParentTreePosition {
	GtkTreeStore*	store;
	GtkTreeIter*	parent;
};

void fill_option_keys(gchar* key, OptionNode* value, ParentTreePosition* pos) {
	GtkTreeIter iter;
	gtk_tree_store_append(pos->store, &iter, pos->parent);
	gtk_tree_store_set( pos->store, &iter
		, 0, key
		, 1, value->current_value
		, 2, TRUE
		, 3, value
		, -1 );
}


void fill_option_groups(gchar* group, GHashTable* options, GtkTreeStore* store) {
	GtkTreeIter iter;
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set( store, &iter
		, 0, group
		, 1, NULL
		, 2, FALSE
		, 3, NULL
		, -1 );

	ParentTreePosition pos = { store, &iter };
	g_hash_table_foreach(options, (GHFunc)fill_option_keys, &pos);
}

void fill_options(GtkTreeStore* store) {
	OptionManager* self = puss_app->option_manager;

	g_hash_table_foreach(self->option_groups, (GHFunc)fill_option_groups, store);
}

SIGNAL_CALLBACK void option_manager_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, GtkWindow* parent) {
	OptionManager* self = puss_app->option_manager;
	GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

	GtkTreeIter iter;
	if( !gtk_tree_model_get_iter(model, &iter, path) )
		return;

	GValue value = { G_TYPE_INVALID };
	gtk_tree_model_get_value(model, &iter, 3, &value);
	OptionNode* option = (OptionNode*)g_value_get_pointer(&value);
	if( !option )
		return;

	OptionSetter setter = option->setter_fun ? option->setter_fun : &puss_default_option_setter;
	if( (*setter)(parent, self->keyfile, option->group, option->key, option->setter_tag) ) {
		option_manager_check_option(option);

		gtk_tree_store_set( GTK_TREE_STORE(model), &iter
			, 1, option->current_value
			, -1 );
	}
}

void puss_option_manager_active() {
	OptionManager* self = puss_app->option_manager;

	// create UI
	GtkBuilder* builder = gtk_builder_new();
	if( !builder )
		return;

	gchar* filepath = g_build_filename(puss_app->module_path, "res", "puss_option_dialog.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(puss) : build config manager ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	GError* err = 0;
	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(puss): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	GtkWidget* dlg = GTK_WIDGET(gtk_builder_get_object(builder, "puss_option_dialog"));
	GtkTreeStore* store = GTK_TREE_STORE(gtk_builder_get_object(builder, "option_store"));
	GtkTreeView* view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "option_treeview"));
	gtk_widget_show_all(GTK_DIALOG(dlg)->vbox);
	gtk_builder_connect_signals(builder, GTK_WINDOW(dlg));
	g_object_unref(G_OBJECT(builder));

	// fill data and show
	gtk_window_set_transient_for(GTK_WINDOW(dlg), puss_app->main_window);

	puss_option_manager_option_reg("puss", "aaa", "abcdef", &puss_default_option_setter, 0);
	puss_option_manager_option_reg("puss", "bbb", "true", 0, "bool");
	puss_option_manager_option_reg("vvvv", "bbb", "true", 0, "bool");
	puss_option_manager_option_reg("aaaa", "bbb", "true", 0, "bool");

	fill_options(store);
	gtk_tree_view_expand_all(view);

	gint res = gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);

	if( self->modified )
		option_manager_save();
}

