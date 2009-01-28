// OptionManager.c
//

#include "OptionManager.h"

#include "Puss.h"
#include "Utils.h"

typedef struct _OptionNotifer OptionNotifer;

struct _OptionNotifer {
	OptionNotifer*	next;
	OptionChanged	fun;
	gpointer		tag;
	GFreeFunc		tag_free_fun;
};

typedef struct _OptionNode {
	Option			option;
	gchar*			value;
	gchar*			default_value;
	OptionNotifer*	notifer_list;
} OptionNode;

static void option_node_free(OptionNode* node) {
	OptionNotifer* p;

	g_free(node->value);
	g_free(node->default_value);

	while( node->notifer_list ) {
		p = node->notifer_list;
		node->notifer_list = p->next;
		if( p->tag_free_fun )
			(*(p->tag_free_fun))(p->tag);
		g_free(p);
	}

	g_free(node);
}

struct _OptionManager {
	gchar*		filepath;
	GKeyFile*	keyfile;
	gboolean	modified;

	GTree*		option_groups;	// group, GTree(key, OptionNode)
};

static gint compare_data_func_wrapper(gpointer a, gpointer b, GCompareFunc cmp) { return (*cmp)(a, b); }

gboolean puss_option_manager_create() {
	OptionManager* self;

	g_assert( !puss_app->option_manager );

	self = g_new0(OptionManager, 1);
	puss_app->option_manager = self;

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

	self->option_groups = g_tree_new_full( (GCompareDataFunc)&compare_data_func_wrapper, (gpointer)&g_ascii_strcasecmp, (GDestroyNotify)&g_free, (GDestroyNotify)&g_tree_destroy );
	if( !self->option_groups ) {
		g_printerr("ERROR(option_manager) : new tree failed!\n");
		return FALSE;
	}

	// now ignore error, TODO :
	// 
	g_key_file_load_from_file(self->keyfile, self->filepath, G_KEY_FILE_KEEP_COMMENTS, 0);

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

void puss_option_manager_destroy() {
	OptionManager* self = puss_app->option_manager;
	if( self && self->modified )
		option_manager_save();

	puss_app->option_manager = 0;
	if( self ) {
		g_key_file_free(self->keyfile);
		g_free(self->filepath);
		g_tree_destroy(self->option_groups);

		g_free(self);
	}
}

const Option* puss_option_manager_option_reg( const gchar* group, const gchar* key, const gchar* default_value ) {
	OptionNode* node;
	OptionManager* self = puss_app->option_manager;

	gchar* ptr_group = 0;
	GTree* option_group = 0;

	if( !g_tree_lookup_extended(self->option_groups, group, (gpointer*)&ptr_group, (gpointer*)&option_group) ) {
		option_group = g_tree_new_full((GCompareDataFunc)&compare_data_func_wrapper, (gpointer)&g_ascii_strcasecmp, (GDestroyNotify)&g_free, (GDestroyNotify)&option_node_free );
		if( !option_group )
			return 0;
		ptr_group = g_strdup(group);
		g_tree_insert(self->option_groups, ptr_group, option_group);
	}

	node = (OptionNode*)g_tree_lookup(option_group, key);
	if( !node ) {
		node = g_new0(OptionNode, 1);
		node->option.group = ptr_group;
		node->option.key = g_strdup(key);

		node->option.value = g_key_file_get_string(self->keyfile, group, key, 0);
		if( default_value ) {
			node->option.default_value = g_strdup(default_value);
			if( !node->option.value )
				node->option.value = g_strdup(default_value);
		}

		g_tree_insert(option_group, (gpointer)node->option.key, node);
	}

	return &node->option;
}

const Option* puss_option_manager_option_find(const gchar* group, const gchar* key) {
	OptionNode* node;
	OptionManager* self = puss_app->option_manager;
	GTree* option_group = (GTree*)g_tree_lookup(self->option_groups, group);
	if( option_group ) {
		node = (OptionNode*)g_tree_lookup(option_group, key);
		if( node )
			return &(node->option);
	}

	return 0;
}

void puss_option_manager_option_set(const Option* option, const gchar* value) {
	gchar* old;
	OptionNotifer* p;
	OptionManager* self = puss_app->option_manager;
	OptionNode* node = (OptionNode*)option;

	if( !g_str_equal(node->option.value, value) ) {
		old = node->option.value;
		node->option.value = g_strdup(value);
		g_key_file_set_string(self->keyfile, option->group, option->key, node->option.value);
		self->modified = TRUE;

		for( p = node->notifer_list; p; p = p->next ) {
			if( p->fun ) {
				p->fun(option, old, p->tag);
			} else  {
				OptionNotifer* t = p;
				p = p->next;
				if( t==node->notifer_list )
					node->notifer_list = p;
				g_free(t);
			}
		}

		g_free(old);
	}
}

gpointer puss_option_manager_monitor_reg(const Option* option, OptionChanged fun, gpointer tag, GFreeFunc tag_free_fun) {
	OptionNotifer* notifer = 0;
	OptionNode* node = (OptionNode*)option;

	if( fun ) {
		notifer = g_new0(OptionNotifer, 1);
		notifer->next = node->notifer_list;
		notifer->fun = fun;
		notifer->tag = tag;
		notifer->tag_free_fun = tag_free_fun;
		node->notifer_list = notifer;
	} else {
		if( tag_free_fun )
			tag_free_fun(tag);
	}

	return notifer;
}

void puss_option_manager_monitor_unreg(gpointer handler) {
	OptionNotifer* p = (OptionNotifer*)handler; 
	if( p ) {
		p->fun = 0;
		if( p->tag_free_fun ) {
			p->tag_free_fun( p->tag );
			p->tag_free_fun = 0;
		}
	}
}

/*
typedef struct _ParentTreePosition {
	GtkTreeStore*	store;
	GtkTreeIter*	parent;
} ParentTreePosition;

gboolean fill_option_keys(gchar* key, OptionNode* value, ParentTreePosition* pos) {
	GtkTreeIter iter;
	gtk_tree_store_append(pos->store, &iter, pos->parent);
	gtk_tree_store_set( pos->store, &iter
		, 0, key
		, 1, value->option.value
		, 2, TRUE
		, 3, value
		, -1 );

	return FALSE;
}

gboolean fill_option_groups(gchar* group, GTree* options, GtkTreeStore* store) {
	GtkTreeIter iter;
	ParentTreePosition pos;

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set( store, &iter
		, 0, group
		, 1, NULL
		, 2, FALSE
		, 3, NULL
		, -1 );

	pos.store = store;
	pos.parent = &iter;

	g_tree_foreach(options, (GTraverseFunc)&fill_option_keys, &pos);

	return FALSE;
}

void fill_options(GtkTreeStore* store) {
	OptionManager* self = puss_app->option_manager;

	g_tree_foreach(self->option_groups, (GTraverseFunc)&fill_option_groups, store);
}

gboolean default_option_setter(GtkWindow* parent, Option* option, gpointer tag);

SIGNAL_CALLBACK void option_manager_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, GtkWindow* parent) {
	GtkTreeIter iter;
	OptionSetter setter;
	OptionNode* node = 0;
	//OptionManager* self = puss_app->option_manager;
	GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

	if( !gtk_tree_model_get_iter(model, &iter, path) )
		return;

	gtk_tree_model_get(model, &iter, 3, &node, -1);
	if( !node )
		return;

	setter = node->setter_fun ? node->setter_fun : &default_option_setter;
	if( (*setter)(parent, &node->option, node->setter_tag) ) {
		option_manager_notify_option_changed(node);

		gtk_tree_store_set( GTK_TREE_STORE(model), &iter
			, 1, node->option.value
			, -1 );
	}
}

void puss_option_manager_active() {
	gchar* filepath;
	GtkBuilder* builder;
	GtkWidget* dlg;
	GtkTreeStore* store;
	GtkTreeView* view;
	GError* err = 0;
	OptionManager* self = puss_app->option_manager;

	// create UI
	builder = gtk_builder_new();
	if( !builder )
		return;

	filepath = g_build_filename(puss_app->module_path, "res", "puss_option_dialog.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(puss) : build config manager ui filepath failed!\n");
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
	store = GTK_TREE_STORE(gtk_builder_get_object(builder, "option_store"));
	view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "option_treeview"));
	gtk_widget_show_all(GTK_DIALOG(dlg)->vbox);
	gtk_builder_connect_signals(builder, GTK_WINDOW(dlg));
	g_object_unref(G_OBJECT(builder));

	// fill data and show
	gtk_window_set_transient_for(GTK_WINDOW(dlg), puss_app->main_window);

	fill_options(store);
	gtk_tree_view_expand_all(view);

	//gint res = 
	gtk_dialog_run(GTK_DIALOG(dlg));

	gtk_widget_destroy(dlg);

	if( self->modified )
		option_manager_save();
}

//-------------------------------------------------------------------------------------------

const gint RESPONSE_USE_DEFFAULT_VALUE = 100;

gboolean default_font_option_setter(GtkWindow* parent, Option* option) {
	gint res;
	gchar* value;
	GtkWidget* dlg = gtk_font_selection_dialog_new( _("font selecting...") );
	if( option->value )
		gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dlg), option->value);
	gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_REVERT_TO_SAVED, RESPONSE_USE_DEFFAULT_VALUE);
	gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);

	res = gtk_dialog_run( GTK_DIALOG(dlg) );

	if( res==GTK_RESPONSE_OK ) {
		value = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dlg));
		if( value && option->value && g_str_equal(value, option->value) ) {
			g_free(value);
			res = GTK_RESPONSE_CANCEL;

		} else {
			g_free(option->value);
			option->value = value;
		}

	} else if( res==RESPONSE_USE_DEFFAULT_VALUE ) {
		if( option->value && option->default_value && g_str_equal(option->value, option->default_value) ) {
			res = GTK_RESPONSE_CANCEL;
		} else {
			g_free(option->value);
			option->value = option->default_value ? g_strdup(option->default_value) : 0;
			res = GTK_RESPONSE_OK;
		}
	}

	gtk_widget_destroy(dlg);

	return res==GTK_RESPONSE_OK;
}

GtkDialog* __create_default_option_setter_dialog(GtkWindow* parent, Option* option, GtkWidget* setter) {
	GtkWidget* label;
	GtkWidget* hbox;
	GtkWidget* frame;
	GtkWidget* dlg = gtk_dialog_new_with_buttons( _("option setting...")
		, parent
		, GTK_DIALOG_MODAL
		, GTK_STOCK_OK, GTK_RESPONSE_OK
		, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
		, GTK_STOCK_REVERT_TO_SAVED, RESPONSE_USE_DEFFAULT_VALUE
		, NULL );

	label = gtk_label_new(option->key);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), setter, TRUE, TRUE, 0);

	frame = gtk_frame_new(option->group);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show_all(frame);

	return GTK_DIALOG(dlg);
}

gboolean __revert_to_default_option(Option* option) {
	if( option->value && option->default_value && g_str_equal(option->value, option->default_value) )
		return FALSE;

	g_free(option->value);
	option->value = option->default_value ? g_strdup(option->default_value) : 0;
	return TRUE;
}

gboolean default_text_option_setter(GtkWindow* parent, Option* option) {
	gint res;
	gchar* value;
	GtkTextIter ps, pe;
	GtkDialog* dlg;
	GtkTextBuffer* buf;
	GtkWidget* view = gtk_text_view_new();
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(view), 3);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_text_buffer_set_text(buf, option->value, -1);

	dlg = __create_default_option_setter_dialog(parent, option, view);

	res = gtk_dialog_run(dlg);

	if( res==GTK_RESPONSE_OK ) {
		gtk_text_buffer_get_start_iter(buf, &ps);
		gtk_text_buffer_get_end_iter(buf, &pe);
		value = gtk_text_buffer_get_text(buf, &ps, &pe, TRUE);
		if( value && option->value && g_str_equal(value, option->value) ) {
			g_free(value);
			res = GTK_RESPONSE_CANCEL;
		} else {
			g_free(option->value);
			option->value = value;
		}

	} else if( res==RESPONSE_USE_DEFFAULT_VALUE ) {
		if( __revert_to_default_option(option) )
			res = GTK_RESPONSE_OK;
	}

	gtk_widget_destroy(GTK_WIDGET(dlg));

	return res==GTK_RESPONSE_OK;
}

gboolean default_enum_option_setter(GtkWindow* parent, Option* option, const gchar* elems) {
	gchar** items;
	gchar** p;
	gint res;
	GtkDialog* dlg;
	gchar* value;
	gint i = 0;
	gint index = -1;
	GtkWidget* combo = gtk_combo_box_new_text();

	items = g_strsplit_set(elems, ",; \t", 0);
	for( p=items; *p; ++p ) {
		if( *p[0] ) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo), *p);
			if( index < 0 && option->value ) {
				if( g_str_equal(*p, option->value) )
					index = i;
				++i;
			}
		}
	}
	g_strfreev(items);

	if( option->value )
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), index);

	dlg = __create_default_option_setter_dialog(parent, option, combo);

	res = gtk_dialog_run(dlg);

	if( res==GTK_RESPONSE_OK ) {
		value = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
		if( value && option->value && g_str_equal(value, option->value) ) {
			g_free(value);
			res = GTK_RESPONSE_CANCEL;
		} else {
			g_free(option->value);
			option->value = value;
		}

	} else if( res==RESPONSE_USE_DEFFAULT_VALUE ) {
		if( __revert_to_default_option(option) )
			res = GTK_RESPONSE_OK;
	}

	gtk_widget_destroy(GTK_WIDGET(dlg));

	return res==GTK_RESPONSE_OK;
}

gboolean default_string_option_setter(GtkWindow* parent, Option* option) {
	gint res;
	GtkDialog* dlg;
	const gchar* value;

	GtkWidget* entry = gtk_entry_new();
	if( option->value )
		gtk_entry_set_text(GTK_ENTRY(entry), option->value);

	dlg = __create_default_option_setter_dialog(parent, option, entry);

	res = gtk_dialog_run(dlg);

	if( res==GTK_RESPONSE_OK ) {
		value = gtk_entry_get_text(GTK_ENTRY(entry));
		if( value && option->value && g_str_equal(value, option->value) ) {
			res = GTK_RESPONSE_CANCEL;
		} else {
			g_free(option->value);
			option->value = value ? g_strdup(value) : 0;
		}

	} else if( res==RESPONSE_USE_DEFFAULT_VALUE ) {
		if( __revert_to_default_option(option) )
			res = GTK_RESPONSE_OK;
	}

	gtk_widget_destroy(GTK_WIDGET(dlg));

	return res==GTK_RESPONSE_OK;
}

gboolean default_option_setter(GtkWindow* parent, Option* option, gpointer tag) {
	const gchar* type = (const gchar*)tag;
	if( type ) {
		if( g_str_equal(type, "font") )
			return default_font_option_setter(parent, option);

		if( g_str_equal(type, "text") )
			return default_text_option_setter(parent, option);

		if( g_str_has_prefix(type, "enum:") )
			return default_enum_option_setter(parent, option, type+5);
	}

	return default_string_option_setter(parent, option);
}
*/
