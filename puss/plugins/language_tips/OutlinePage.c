// OutlinePage.c
// 

#include "LanguageTips.h"

#include <assert.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

typedef struct {
	LanguageTips*	self;
	GtkTreeIter*	iter;
} AddItemTag;

static void outline_add_elem(CppElem* elem, AddItemTag* parent) {
	GtkTreeIter iter;

	if( elem->type==CPP_ET_INCLUDE || elem->type==CPP_ET_UNDEF )
		return;

	gtk_tree_store_append(parent->self->outline_store, &iter, parent->iter);
	gtk_tree_store_set( parent->self->outline_store, &iter
		, 0, parent->self->icons[elem->type]
		, 1, elem->name->buf
		, 2, elem
		, -1 );

	switch( elem->type ) {
	case CPP_ET_CLASS:
	case CPP_ET_ENUM:
	case CPP_ET_NAMESPACE:
	case CPP_ET_NCSCOPE:
		{
			AddItemTag tag = {parent->self, &iter};
			g_list_foreach( cpp_elem_get_subscope(elem), (GFunc)outline_add_elem, &tag );
		}
		break;
	}
}

static void locate_line(LanguageTips* self, size_t line, GtkTreeIter* parent) {
	CppElem* elem;
	GtkTreePath* path;
	GtkTreeSelection* sel;
	GtkTreeIter iter;

	if( !gtk_tree_model_iter_children(GTK_TREE_MODEL(self->outline_store), &iter, parent) )
		return;

	do {
		gtk_tree_model_get(GTK_TREE_MODEL(self->outline_store), &iter, 2, &elem, -1);
		assert( elem );

		if( line < elem->sline )
			break;

		else if( line > elem->eline )
			continue;

		sel = gtk_tree_view_get_selection(self->outline_view);
		gtk_tree_selection_select_iter(sel, &iter);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(self->outline_store), &iter);
		gtk_tree_view_expand_to_path(self->outline_view, path);
		gtk_tree_view_scroll_to_cell(self->outline_view, path, NULL, FALSE, 0.0f, 0.0f);
		if( gtk_tree_model_iter_has_child(GTK_TREE_MODEL(self->outline_store), &iter) )
			locate_line(self, line, &iter);
		break;

	} while( gtk_tree_model_iter_next(GTK_TREE_MODEL(self->outline_store), &iter) );
}

static void outline_set_file(LanguageTips* self, CppFile* file, gint line) {
	if( file != self->outline_file ) {
		self->outline_pos = -1;	// reset outline_pos

		gtk_tree_view_set_model(self->outline_view, 0);
		gtk_tree_store_clear(self->outline_store);

		if( self->outline_file ) {
			cpp_file_unref(self->outline_file);
			self->outline_file = 0;
		}

		if( file ) {
			AddItemTag tag = {self, 0};

			self->outline_file = cpp_file_ref(file);
			g_list_foreach( file->root_scope.v_ncscope.scope, (GFunc)outline_add_elem, &tag );
		}

		gtk_tree_view_set_model(self->outline_view, GTK_TREE_MODEL(self->outline_store));
	}

	if( file && self->outline_pos != line ) {
		self->outline_pos = line;
		gtk_tree_selection_unselect_all( gtk_tree_view_get_selection(self->outline_view) );

		locate_line(self, line + 1, 0);
	}
}

static gboolean do_outline_update(LanguageTips* self) {
	GtkNotebook* doc_panel;
	gint num;
	GtkTextBuffer* buf;
	GString* url;
	CppFile* file;
	GtkTextIter iter;

	doc_panel = puss_get_doc_panel(self->app);
	num = gtk_notebook_get_current_page(doc_panel);
	if( num < 0 )
		return FALSE;

	buf = self->app->doc_get_buffer_from_page_num(num);
	if( !buf )
		return FALSE;

	url = self->app->doc_get_url(buf);
	if( !url )
		return FALSE;

	file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
	if( !file )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	num = gtk_text_iter_get_line(&iter);

	outline_set_file(self, file, num);
	cpp_file_unref(file);

	return TRUE;
}

void outline_update(LanguageTips* self) {
	if( !do_outline_update(self) )
		outline_set_file(self, 0, 0);
}

SIGNAL_CALLBACK gboolean outline_cb_query_tooltip( GtkTreeView* tree_view
	, gint x
	, gint y
	, gboolean keyboard_mode
	, GtkTooltip* tooltip
	, LanguageTips* self )
{
	GtkTreeModel* model;
	GtkTreePath* path;
	GtkTreeIter iter;
	CppElem* elem = 0;
	if( gtk_tree_view_get_tooltip_context(tree_view, &x, &y, keyboard_mode, &model, &path, &iter) ) {
		gtk_tree_model_get(model, &iter, 2, &elem, -1);
		if( elem && elem->decl) {
			gtk_tooltip_set_text(tooltip, elem->decl->buf);
			return TRUE;
		}
	}

	return FALSE;
}

SIGNAL_CALLBACK void outline_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, LanguageTips* self) {
	GtkTreeIter iter;
	CppElem* elem = 0;

	if( !gtk_tree_model_get_iter(GTK_TREE_MODEL(self->outline_store), &iter, path) )
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(self->outline_store), &iter, 2, &elem, -1);
	open_and_locate_elem(self, elem);
}

