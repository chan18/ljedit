// Tips.cpp
// 

#include "Tips.h"

#include <glib/gi18n.h>

#include "LJCS.h"

struct Tips {
	Puss*			app;
	Environ*			env;
	Icons*			icons;

	GtkWidget*		include_window;
	GtkTreeView*	include_view;
	GtkTreeModel*	include_model;
	StringSet*		include_files;

	GtkWidget*		list_window;
	GtkTreeView*	list_view;
	GtkTreeModel*	list_model;

	GtkWidget*		decl_window;
	GtkTextView*	decl_view;
	GtkTextBuffer*	decl_buffer;
};

SIGNAL_CALLBACK void tips_include_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, Tips* self) {
}

SIGNAL_CALLBACK void tips_list_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, Tips* self) {
}

gboolean init_tips(Tips* self, Puss* app, Environ* env, Icons* icons) {
	self->app = app;
	self->env = env;
	self->icons = icons;

	// init UI
	GtkBuilder* builder = gtk_builder_new();
	if( !builder )
		return FALSE;

	gchar* filepath = g_build_filename(app->get_module_path(), "extends", "ljcs_res", "tips_ui.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(ljcs) : build tips ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	GError* err = 0;
	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(ljcs): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	self->include_window = GTK_WIDGET(gtk_builder_get_object(builder, "include_window"));
	self->include_view   = GTK_TREE_VIEW(gtk_builder_get_object(builder, "include_view"));
	self->include_model  = GTK_TREE_MODEL(gtk_builder_get_object(builder, "include_store"));

	self->list_window    = GTK_WIDGET(gtk_builder_get_object(builder, "list_window"));
	self->list_view      = GTK_TREE_VIEW(gtk_builder_get_object(builder, "list_view"));
	self->list_model     = GTK_TREE_MODEL(gtk_builder_get_object(builder, "list_store"));

	self->decl_window    = GTK_WIDGET(gtk_builder_get_object(builder, "decl_window"));
	self->decl_view      = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "decl_view"));
	self->decl_buffer    = set_cpp_lang_to_source_view(self->decl_view);

	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "include_panel")));
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "list_panel")));
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "decl_panel")));

	g_object_unref(G_OBJECT(builder));

	//gtk_widget_show(tip_window);

	return TRUE;
}

gboolean decref_each_file(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, Tips* self) {
	GValue value = { G_TYPE_INVALID };
	gtk_tree_model_get_value(model, iter, 3, &value);
	cpp::Element* elem = (cpp::Element*)g_value_get_pointer(&value);
	g_assert( elem );

	self->env->file_decref(&(elem->file));

	return FALSE;
}

void clear_list_store(Tips* self) {
	gtk_tree_model_foreach(self->list_model, (GtkTreeModelForeachFunc)&decref_each_file, self);
	gtk_list_store_clear(GTK_LIST_STORE(self->list_model));
}

struct ElemIndexByName : public std::binary_function<cpp::Element*, cpp::Element*, bool>
{
	bool operator()(const cpp::Element* left, const cpp::Element* right) const
		{ return left->name < right->name; }
};

typedef std::set<cpp::Element*, ElemIndexByName>	ElementNameIndexSet;

void fill_list_store(Tips* self, cpp::ElementSet& mset) {
	ElementNameIndexSet iset;
	{
		cpp::ElementSet::iterator it = mset.begin();
		cpp::ElementSet::iterator end = mset.end();
		for( ; it!=end; ++it ) {
			ElementNameIndexSet::iterator p = iset.find(*it);
			if( p==iset.end() )
				iset.insert(*it);

			else if( (*it)->type==cpp::ET_CLASS )
				*p = *it;
		}
	}

	// fill
	if( !iset.empty() ) {
		gtk_tree_view_set_model(self->list_view, 0);

		ElementNameIndexSet::iterator it = iset.begin();
		ElementNameIndexSet::iterator end = iset.end();
		for( size_t i=0; it!=end && i < 250; ++it ) {
			++i;

			cpp::Element* elem = *it;
			self->env->file_incref(&elem->file);

			GtkTreeIter iter;
			gtk_list_store_append( GTK_LIST_STORE(self->list_model), &iter );
			gtk_list_store_set( GTK_LIST_STORE(self->list_model), &iter
				, 0, self->icons->get_icon_from_elem(*elem)
				, 1, elem->name.c_str()
				, 2, elem->decl.c_str()
				, 3, elem
				, -1 );
		}

		gtk_tree_view_set_model(self->list_view, self->list_model);
	}
}

Tips* tips_create(Puss* app, Environ* env, Icons* icons) {
	Tips* self = g_new0(Tips, 1);
	if( self ) {
		if( !init_tips(self, app, env, icons) ) {
			g_free(self);
			self = 0;
		}

		g_object_ref(self->list_model);
		g_object_ref(self->include_model);
		self->include_files = new StringSet();
	}

	return self;
}

void tips_destroy(Tips* self) {
	if( self ) {
		g_object_unref(self->list_model);
		g_object_unref(self->include_model);

		gtk_widget_destroy(self->list_window);
		gtk_widget_destroy(self->include_window);
		gtk_widget_destroy(self->decl_window);

		delete self->include_files;

		g_free(self);
		self = 0;
	}
}

gboolean tips_include_is_visible(Tips* self) {
	return GTK_WIDGET_VISIBLE(self->include_window);
}

gboolean tips_list_is_visible(Tips* self) {
	return GTK_WIDGET_VISIBLE(self->list_window);
}

gboolean tips_decl_is_visible(Tips* self) {
	return GTK_WIDGET_VISIBLE(self->decl_window);
}

void tips_include_tip_show(Tips* self, gint x, gint y, StringSet& files) {
	tips_list_tip_hide(self);
	tips_decl_tip_hide(self);

	gtk_list_store_clear(GTK_LIST_STORE(self->include_model));

	if( files.empty() ) {
		tips_include_tip_hide(self);
		return;
	}

	self->include_files->swap(files);

	GtkTreeIter iter;
	StringSet::iterator it = self->include_files->begin();
	StringSet::iterator end = self->include_files->end();
	for( ; it!=end; ++it ) {
		gtk_list_store_append( GTK_LIST_STORE(self->include_model), &iter );
		gtk_list_store_set( GTK_LIST_STORE(self->include_model), &iter
			, 0, it->c_str()
			, -1 );
	}

	if( gtk_tree_model_get_iter_first(self->include_model, &iter) ) {
		GtkTreeSelection* sel = gtk_tree_view_get_selection(self->include_view);
		gtk_tree_selection_select_iter(sel, &iter);
	}

	gtk_tree_view_columns_autosize(self->include_view);

	gtk_window_move(GTK_WINDOW(self->include_window), x, y);
	gtk_window_resize(GTK_WINDOW(self->include_window), 200, 100);
	gtk_container_resize_children(GTK_CONTAINER(self->include_window));
	gtk_widget_show(self->include_window);
}

void tips_include_tip_hide(Tips* self) {
	gtk_widget_hide(self->include_window);
}

void tips_list_tip_show(Tips* self, gint x, gint y, cpp::ElementSet& mset) {
	clear_list_store(self);

	if( mset.empty() ) {
		gtk_widget_hide(self->list_window);
		return;
	}

	fill_list_store(self, mset);

	GtkTreeIter iter;
	if( gtk_tree_model_get_iter_first(self->list_model, &iter) ) {
		GtkTreeSelection* sel = gtk_tree_view_get_selection(self->list_view);
		gtk_tree_selection_select_iter(sel, &iter);
	}

	gtk_tree_view_columns_autosize(self->list_view);

	if( tips_decl_is_visible(self) ) {
		gint w = 0;
		gint h = 0;
		gtk_window_get_size(GTK_WINDOW(self->list_window), &w, &h);
		if( y >= (h + 16) ) {
			y -= (h + 16);

		} else {
			y += (h + 5);
			//y += ( ( tip_.decl_window().get_height() + 5 );
		}
	}

	gtk_window_move(GTK_WINDOW(self->list_window), x, y);
	//gtk_window_resize(GTK_WINDOW(self->list_window), 200, 100);
	//gtk_container_resize_children(GTK_CONTAINER(self->list_window));
	gtk_widget_show(self->list_window);
}

void tips_list_tip_hide(Tips* self) {
	gtk_widget_hide(self->list_window);
}

void tips_decl_tip_show(Tips* self, gint x, gint y, cpp::ElementSet& mset) {
	if( mset.empty() ) {
		tips_decl_tip_hide(self);
		return;
	}

	gchar str[64];
	str[0] = '\0';

	gtk_text_buffer_set_text(self->decl_buffer, str, 0);

	cpp::ElementSet::iterator it = mset.begin();
	cpp::ElementSet::iterator end = mset.end();
	for( ; it!=end; ++it ) {
		cpp::Element* elem = *it;

		gtk_text_buffer_insert_at_cursor(self->decl_buffer, elem->decl.c_str(), (gint)elem->decl.size());
		gtk_text_buffer_insert_at_cursor(self->decl_buffer, "\n    // ", -1);
		gtk_text_buffer_insert_at_cursor(self->decl_buffer, elem->file.filename.c_str(), (gint)elem->file.filename.size());
		g_snprintf(str, 64, ":%d\n\n", elem->sline);
		gtk_text_buffer_insert_at_cursor(self->decl_buffer, str, -1);
	}

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_offset(self->decl_buffer, &iter, 0);
	gtk_text_buffer_place_cursor(self->decl_buffer, &iter);

	gtk_window_move(GTK_WINDOW(self->decl_window), x, y);
	gtk_window_resize(GTK_WINDOW(self->decl_window), 200, 100);
	gtk_container_resize_children(GTK_CONTAINER(self->decl_window));
	gtk_widget_show(self->decl_window);
}

void tips_decl_tip_hide(Tips* self) {
	gtk_widget_hide(self->decl_window);
}

gboolean tips_locate_sub(Tips* self, gint x, gint y, const gchar* key) {
	GtkTreeIter iter;
	if( !gtk_tree_model_iter_children(self->list_model, &iter, NULL) )
		return FALSE;

	do {
		GValue value = { G_TYPE_INVALID };
		gtk_tree_model_get_value(self->list_model, &iter, 1, &value);
		cpp::Element* elem = (cpp::Element*)g_value_get_pointer(&value);
		g_assert( elem );

		if( elem->name.find(key)==0 ) {
			GtkTreeSelection* sel = gtk_tree_view_get_selection(self->list_view);
			gtk_tree_selection_select_iter(sel, &iter);
			GtkTreePath* path = gtk_tree_model_get_path(self->list_model, &iter);
			gtk_tree_view_scroll_to_cell(self->list_view, path, NULL, FALSE, 0.0f, 0.0f);
			return TRUE;
		}

	} while( gtk_tree_model_iter_next(self->list_model, &iter) );

	return FALSE;
}

cpp::Element* tips_list_get_selected(Tips* self) {
    cpp::Element* result = 0;

	GtkTreeIter iter;
	GtkTreeSelection* sel = gtk_tree_view_get_selection(self->list_view);
	if( gtk_tree_selection_get_selected(sel, &self->list_model, &iter) ) {
		GValue value = { G_TYPE_INVALID };
		gtk_tree_model_get_value(self->list_model, &iter, 3, &value);
		result = (cpp::Element*)g_value_get_pointer(&value);
	}

	return result;
}

const gchar* tips_include_get_selected(Tips* self) {
	const gchar* result = 0;

	GtkTreeIter iter;
	GtkTreeSelection* sel = gtk_tree_view_get_selection(self->include_view);
	if( gtk_tree_selection_get_selected(sel, &self->list_model, &iter) ) {
		GValue value = { G_TYPE_INVALID };
		gtk_tree_model_get_value(self->include_model, &iter, 0, &value);
		result = g_value_get_string(&value);
	}

	return result;
}

void tips_select_next(Tips* self) {
	GtkTreeView* view = 0;
	if( GTK_WIDGET_VISIBLE(self->list_window) )
		view = self->list_view;
	else if( GTK_WIDGET_VISIBLE(self->include_window) )
		view = self->include_view;

	if( !view )
		return;

	GtkTreeModel* model = gtk_tree_view_get_model(view);

	GtkTreeIter iter;
	GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
	if( gtk_tree_selection_get_selected(sel, &model, &iter) ) {
		if( !gtk_tree_model_iter_next(model, &iter) )
			return;

	} else if( !gtk_tree_model_iter_children(model, &iter, NULL) ) {
		return;
	}

	gtk_tree_selection_select_iter(sel, &iter);
	gtk_tree_view_scroll_to_cell(view, gtk_tree_model_get_path(model, &iter), NULL, FALSE, 0.0f, 0.0f);
}

void tips_select_prev(Tips* self) {
	GtkTreeView* view = 0;
	if( GTK_WIDGET_VISIBLE(self->list_window) )
		view = self->list_view;
	else if( GTK_WIDGET_VISIBLE(self->include_window) )
		view = self->include_view;

	if( !view )
		return;

	GtkTreeModel* model = gtk_tree_view_get_model(view);

	GtkTreeIter iter;
	GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
	if( gtk_tree_selection_get_selected(sel, &model, &iter) ) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		if( !gtk_tree_path_prev(path) )
			return;

		if( !gtk_tree_model_get_iter(model, &iter, path) )
			return;

	} else {
		gint count = gtk_tree_model_iter_n_children(model, &iter);
		if( count <= 0 || !gtk_tree_model_iter_nth_child(model, &iter, NULL, (count-1)) )
			return;
	}

	gtk_tree_selection_select_iter(sel, &iter);
	gtk_tree_view_scroll_to_cell(view, gtk_tree_model_get_path(model, &iter), NULL, FALSE, 0.0f, 0.0f);
}

