// OutlinePage.cpp
// 

#include "OutlinePage.h"

#include "LJCS.h"

// TODO : use struct replace class
// 
class OutlinePage {
public:
    gboolean create(Puss* app, Environ* env, Icons* icons) {
		app_ = app;
		env_ = env;
		icons_ = icons;

		// init UI
		GtkBuilder* builder = gtk_builder_new();
		if( !builder )
			return FALSE;

		gchar* filepath = g_build_filename(app_->get_module_path(), "extends", "ljcs_res", "outline_page_ui.xml", NULL);
		if( !filepath ) {
			g_printerr("ERROR(ljcs) : build outline ui filepath failed!\n");
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

		tree_view_ = GTK_TREE_VIEW(gtk_builder_get_object(builder, "outline_treeview"));
		tree_store_ = GTK_TREE_STORE(g_object_ref(gtk_builder_get_object(builder, "outline_store")));

		self_panel_ = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
		gtk_widget_show_all(self_panel_);

		app_->panel_append(self_panel_, gtk_label_new(_("Outline")), "ljcs_outline", PUSS_PANEL_POS_RIGHT);

		gtk_builder_connect_signals(builder, this);
		g_object_unref(G_OBJECT(builder));

		return TRUE;
	}

	void destroy() {
		if( self_panel_ ) {
			app_->panel_remove(self_panel_);
			self_panel_ = 0;
		}

		if( tree_store_ ) {
			g_object_unref(G_OBJECT(tree_store_));
			tree_store_ = 0;
		}

		if( file_ != 0 ) {
			env_->file_decref(file_);
			file_ = 0;
			line_ = 0;
		}
	}

public:
    void set_file(cpp::File* file, int line) {
		if( file != file_ ) {
			gtk_tree_view_set_model(tree_view_, 0);
			gtk_tree_store_clear(tree_store_);

			if( file ) {
				env_->file_incref(file);

				cpp::Elements::iterator it = file->scope.elems.begin();
				cpp::Elements::iterator end = file->scope.elems.end();
				for( ; it!=end; ++it )
					add_elem(*it, 0);
			}

			if( file_ )
				env_->file_decref(file_);

			file_ = file;

			gtk_tree_view_set_model(tree_view_, GTK_TREE_MODEL(tree_store_));
		}

		if( file && line_!=line ) {
			line_ = line;

			GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view_);
			gtk_tree_selection_unselect_all(sel);
			locate_line( (size_t)line + 1, 0 );
		}
	}

public:
	void add_elem(const cpp::Element* elem, GtkTreeIter* parent) {
		switch(elem->type) {
		case cpp::ET_INCLUDE:
		case cpp::ET_UNDEF:
			return;
		}

		GtkTreeIter iter;
		gtk_tree_store_append(tree_store_, &iter, parent);
		gtk_tree_store_set( tree_store_, &iter
			, 0, icons_->get_icon_from_elem(*elem)
			, 1, elem->name.c_str()
			, 2, elem
			, -1 );

		cpp::Scope* scope = 0;
		switch(elem->type) {
		case cpp::ET_ENUM:		scope = &((cpp::Enum*)elem)->scope;			break;
		case cpp::ET_CLASS:		scope = &((cpp::Class*)elem)->scope;		break;
		case cpp::ET_NAMESPACE:	scope = &((cpp::Namespace*)elem)->scope;	break;
		}

		if( scope != 0 ) {
			// add children
			cpp::Elements::iterator it = scope->elems.begin();
			cpp::Elements::iterator end = scope->elems.end();
			for( ; it!=end; ++it )
				add_elem(*it, &iter);
		}
	}

	void locate_line(size_t line, GtkTreeIter* parent) {
		GtkTreeIter iter;
		if( !gtk_tree_model_iter_children(GTK_TREE_MODEL(tree_store_), &iter, parent) )
			return;

		do {
			cpp::Element* elem = 0;
			gtk_tree_model_get(GTK_TREE_MODEL(tree_store_), &iter, 2, &elem, -1);
			g_assert( elem );

			if( line < elem->sline )
				break;

			else if( line > elem->eline )
				continue;

			GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view_);
			gtk_tree_selection_select_iter(sel, &iter);
			GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree_store_), &iter);
			gtk_tree_view_expand_to_path(tree_view_, path);
			gtk_tree_view_scroll_to_cell(tree_view_, path, NULL, FALSE, 0.0f, 0.0f);
			if( gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tree_store_), &iter) )
				locate_line(line, &iter);
			break;

		} while( gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_store_), &iter) );
	}

	bool do_update() {
		GtkNotebook* doc_panel = puss_get_doc_panel(app_);
		gint page_num = gtk_notebook_get_current_page(doc_panel);
		if( page_num < 0 )
			return false;

		GtkTextBuffer* buf = app_->doc_get_buffer_from_page_num(page_num);
		if( !buf )
			return false;

		GString* url = app_->doc_get_url(buf);
		if( !url )
			return false;

		cpp::File* file = env_->find_parsed(std::string(url->str, url->len));
		if( !file )
			return false;

		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
		gint line = gtk_text_iter_get_line(&iter);
		set_file(file, line);

		env_->file_decref(file);
		return true;
	}

public:
	Puss*			app_;
	Environ*		env_;
	Icons*			icons_;

	// UI
	GtkWidget*		self_panel_;
	GtkTreeView*	tree_view_;
	GtkTreeStore*	tree_store_;

	// outline position
    cpp::File*		file_;
    int				line_;
};

SIGNAL_CALLBACK gboolean outline_page_cb_query_tooltip( GtkTreeView* tree_view
	, gint x
	, gint y
	, gboolean keyboard_mode
	, GtkTooltip* tooltip
	, OutlinePage* self )
{
	GtkTreeModel* model;
	GtkTreePath* path;
	GtkTreeIter iter;
	if( gtk_tree_view_get_tooltip_context(tree_view, &x, &y, keyboard_mode, &model, &path, &iter) ) {
		cpp::Element* elem = 0;
		gtk_tree_model_get(model, &iter, 2, &elem, -1);
		if( elem ) {
			gtk_tooltip_set_text(tooltip, elem->decl.c_str());
			return TRUE;
		}
	}

	return FALSE;
}

SIGNAL_CALLBACK void outline_page_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, OutlinePage* self) {
	GtkTreeIter iter;
	if( !gtk_tree_model_get_iter(GTK_TREE_MODEL(self->tree_store_), &iter, path) )
		return;

	cpp::Element* elem = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(self->tree_store_), &iter, 2, &elem, -1);
	if( !elem )
		return;

	self->app_->doc_open(elem->file.filename.c_str(), (gint)elem->sline - 1, -1, FALSE);
}

OutlinePage* outline_page_create(Puss* app, Environ* env, Icons* icons) {
	OutlinePage* self = new OutlinePage();
	if( self ) {
		if( !self->create(app, env, icons) ) {
			delete self;
			self = 0;
		}
	}

	return self;
}

void outline_page_destroy(OutlinePage* self) {
	if( self ) {
		self->destroy();

		delete self;
		self = 0;
	}
}

void outline_page_update(OutlinePage* self) {
	if( !self->do_update() )
		self->set_file(NULL, 0);
}

