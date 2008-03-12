// OutlinePage.cpp
// 

#include "OutlinePage.h"

#include <glib/gi18n.h>

#include "LJCS.h"


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

		GtkWidget*	self_panel = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
		GtkCellLayout* cell_layout = GTK_CELL_LAYOUT(gtk_builder_get_object(builder, "elem_column"));
		GtkCellRenderer* icon_cell_renderer = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "icon_cell_renderer"));
		GtkCellRenderer* decl_cell_renderer = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "decl_cell_renderer"));

		gtk_cell_layout_set_cell_data_func(cell_layout, icon_cell_renderer, (GtkCellLayoutDataFunc)&OutlinePage::cb_icon_cell_get_data, this, 0);
		gtk_cell_layout_set_cell_data_func(cell_layout, decl_cell_renderer, (GtkCellLayoutDataFunc)&OutlinePage::cb_decl_cell_get_data, this, 0);

		tree_view_ = GTK_TREE_VIEW(gtk_builder_get_object(builder, "outline_treeview"));
		tree_store_ = GTK_TREE_STORE(g_object_ref(gtk_builder_get_object(builder, "outline_store")));

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

				// test
				GtkTreeIter iter;
				gtk_tree_store_append(tree_store_, &iter, NULL);
				gtk_tree_store_set(tree_store_, &iter, 0, (gpointer)0, -1, -1);
				/*
				const Gtk::TreeNodeChildren& node = store_->children();
				cpp::Elements::iterator it = file->scope.elems.begin();
				cpp::Elements::iterator end = file->scope.elems.end();
				for( ; it!=end; ++it )
					view_add_elem(*it, node);
				*/
			}

			if( file_ )
				env_->file_decref(file_);

			file_ = file;

			gtk_tree_view_set_model(tree_view_, GTK_TREE_MODEL(tree_store_));
		}

		if( file && line_!=line ) {
			line_ = line;

			/*
			view_.get_selection()->unselect_all();
			const Gtk::TreeNodeChildren& root = store_->children();
			view_locate_line((size_t)(line + 1), root);
			*/
		}
	}

private:
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
	static void cb_icon_cell_get_data( GtkCellLayout* cell_layout
			, GtkCellRenderer* cell
			, GtkTreeModel* tree_model
			, GtkTreeIter* iter
			, OutlinePage* self )
	{
		//self->icons_->get_icon_from_elem();

		GdkPixbuf* icon = gdk_pixbuf_new_from_file("D:/louis/lj/puss/bin/extends/ljcs_res/class.png", NULL);
		GValue value = { GDK_TYPE_PIXBUF };
		value.data->v_pointer = (gpointer)icon;
		g_object_set_property(G_OBJECT(cell), "pixbuf", &value);
		g_object_unref(G_OBJECT(icon));
	}

	static void cb_decl_cell_get_data( GtkCellLayout* cell_layout
			, GtkCellRenderer* cell
			, GtkTreeModel* tree_model
			, GtkTreeIter* iter
			, OutlinePage* self )
	{
		const gchar* text = "vvvvvvvvvvvvvvvvv";
		GValue value = { G_TYPE_STRING };
		value.data->v_pointer = (gpointer)text;
		g_object_set_property(G_OBJECT(cell), "text", &value);
	}

private:
	Puss*		app_;
	Environ*	env_;
	Icons*		icons_;

	// UI
	GtkTreeView*	tree_view_;
	GtkTreeStore*	tree_store_;

	// outline position
    cpp::File*		file_;
    int				line_;
};

SIGNAL_CALLBACK void outline_page_cb_row_activated(GtkTreePath* path, GtkTreeViewColumn* col, OutlinePage* self) {
	g_print("vvvvvvvv\n");

	/*
	GtkTreeModelIfa::iterator it = store_->get_iter(path);
    const cpp::Element* elem = it->get_value(columns_.elem);
	if( elem )
		app_->doc_open(elem->filename.c_str(), (elem->sline - 1), 0);
	*/
}

/*
void OutlinePage::view_add_elem(const cpp::Element* elem, const Gtk::TreeNodeChildren& parent) {
    Gtk::TreeRow row = *(store_->append(parent));
	row[columns_.icon] = LJCSIcons::self().get_icon_from_elem(*elem);
    row[columns_.decl] = elem->decl;
    row[columns_.elem] = elem;

    cpp::Scope* scope = 0;
    switch(elem->type) {
    case cpp::ET_ENUM:		scope = &((cpp::Enum*)elem)->scope;			break;
    case cpp::ET_CLASS:		scope = &((cpp::Class*)elem)->scope;		break;
    case cpp::ET_NAMESPACE:	scope = &((cpp::Namespace*)elem)->scope;	break;
    }

    if( scope != 0 ) {
        // add children
        const Gtk::TreeNodeChildren& node = row.children();
        cpp::Elements::iterator it = scope->elems.begin();
        cpp::Elements::iterator end = scope->elems.end();
        for( ; it!=end; ++it )
            view_add_elem(*it, node);
    }
}
*/

/*
void OutlinePage::view_locate_line(size_t line, const Gtk::TreeNodeChildren& parent) {
    Gtk::TreeStore::iterator it = parent.begin();
    Gtk::TreeStore::iterator end = parent.end();
    for( ; it!=end; ++it ) {
        const cpp::Element* elem = it->get_value(columns_.elem);
        assert( elem != 0 );

        if( line < elem->sline )
            break;

        else if( line > elem->eline )
            continue;

        view_.get_selection()->select(it);
        view_.expand_to_path(store_->get_path(it));
        view_.scroll_to_row(store_->get_path(it));
        if( !it->children().empty() )
            view_locate_line( line, it->children() );
        break;
    }
}
*/

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

