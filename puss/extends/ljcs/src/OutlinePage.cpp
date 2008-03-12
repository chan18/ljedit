// OutlinePage.cpp
// 

#include "OutlinePage.h"

#include <glib/gi18n.h>

#include "LJCS.h"


SIGNAL_CALLBACK void outline_page_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, OutlinePage* self);

class OutlinePage {
	friend void outline_page_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, OutlinePage* self) ;
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
			g_printerr("ERROR(ljcs) : build ui filepath failed!\n");
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

		GtkWidget*	self_panel = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
		gtk_widget_show_all(self_panel);
		gtk_notebook_append_page(puss_get_right_panel(app_), self_panel, gtk_label_new(_("Outline")));

		gtk_builder_connect_signals(builder, this);
		g_object_unref(G_OBJECT(builder));

		g_timeout_add(1000, (GSourceFunc)&OutlinePage::outline_update_timeout, this);

		return TRUE;
	}

	void destroy() {
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

			if( file ) {
				env_->file_incref(file);

				gtk_tree_store_clear(tree_store_);
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

private:
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
			, 2, elem->decl.c_str()
			, 3, elem
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
			GValue value = { G_TYPE_INVALID };
			gtk_tree_model_get_value(GTK_TREE_MODEL(tree_store_), &iter, 3, &value);
			cpp::Element* elem = (cpp::Element*)g_value_get_pointer(&value);
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
			if( !gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tree_store_), &iter) )
				locate_line(line, &iter);
			break;

		} while( gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_store_), &iter) );
	}

	static gboolean outline_update_timeout(OutlinePage* self) {
		GtkNotebook* doc_panel = puss_get_doc_panel(self->app_);
		gint page_num = gtk_notebook_get_current_page(doc_panel);
		if( page_num < 0 )
			return TRUE;

		GtkTextBuffer* buf = self->app_->doc_get_buffer_from_page_num(page_num);
		if( !buf )
			return TRUE;

		GString* url = self->app_->doc_get_url(buf);
		if( !url )
			return TRUE;

		cpp::File* file = self->env_->find_parsed(std::string(url->str, url->len));
		if( !file )
			return TRUE;

		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
		gint line = gtk_text_iter_get_line(&iter);
		self->set_file(file, line);

		self->env_->file_decref(file);
		return TRUE;
	}

private:
	Puss*			app_;
	Environ*		env_;
	Icons*			icons_;

	// UI
	GtkTreeView*	tree_view_;
	GtkTreeStore*	tree_store_;

	// outline position
    cpp::File*		file_;
    int				line_;
};

SIGNAL_CALLBACK void outline_page_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, OutlinePage* self) {
	GtkTreeIter iter;
	if( !gtk_tree_model_get_iter(GTK_TREE_MODEL(self->tree_store_), &iter, path) )
		return;

	GValue value = { G_TYPE_INVALID };
	gtk_tree_model_get_value(GTK_TREE_MODEL(self->tree_store_), &iter, 3, &value);
	const cpp::Element* elem = (const cpp::Element*)g_value_get_pointer(&value);
	if( !elem )
		return;

	self->app_->doc_open(elem->file.filename.c_str(), (gint)elem->sline - 1, -1);
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

void preview_page_set_file(OutlinePage* self, cpp::File* file, gint line) {
	assert( self );

	self->set_file(file, line);
}

