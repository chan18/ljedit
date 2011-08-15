// Tips.c
// 

#include "LanguageTips.h"

#include <assert.h>

// TODO : use tooltip replace tips_decl_window
//		since gtk-2.12, tooltip can use markup language
// 

void tips_init(LanguageTips* self) {
}

void tips_final(LanguageTips* self) {
	if( self->tips_include_files ) {
		gtk_list_store_clear(GTK_LIST_STORE(self->tips_include_model));
		g_list_foreach(self->tips_include_files, (GFunc)g_free, 0);
		g_list_free(self->tips_include_files);
	}

	if( self->tips_list_seq )
		g_sequence_free(self->tips_list_seq);

	if( self->tips_decl_seq )
		g_sequence_free(self->tips_decl_seq);
}

void tips_include_tip_show(LanguageTips* self, gint x, gint y, GList* files) {
	GList* p;
	GtkTreeIter iter;
	GtkTreeSelection* sel;

	tips_list_tip_hide(self);
	tips_decl_tip_hide(self);

	if( self->tips_include_files ) {
		gtk_list_store_clear(GTK_LIST_STORE(self->tips_include_model));
		g_list_foreach(self->tips_include_files, (GFunc)g_free, 0);
		g_list_free(self->tips_include_files);
	}

	self->tips_include_files = files;
	if( !files ) {
		tips_include_tip_hide(self);
		return;
	}

	gtk_tree_view_set_model(self->tips_include_view, 0);
	for( p=files; p; p=p->next ) {
		gtk_list_store_append( GTK_LIST_STORE(self->tips_include_model), &iter );
		gtk_list_store_set( GTK_LIST_STORE(self->tips_include_model), &iter
			, 0, p->data
			, -1 );
	}
	gtk_tree_view_set_model(self->tips_include_view, self->tips_include_model);

	if( gtk_tree_model_get_iter_first(self->tips_include_model, &iter) ) {
		sel = gtk_tree_view_get_selection(self->tips_include_view);
		gtk_tree_selection_select_iter(sel, &iter);
	}

	gtk_tree_view_columns_autosize(self->tips_include_view);

	gtk_window_move(GTK_WINDOW(self->tips_include_window), x, y);
	gtk_window_resize(GTK_WINDOW(self->tips_include_window), 200, 100);
	gtk_container_resize_children(GTK_CONTAINER(self->tips_include_window));
	gtk_widget_show(self->tips_include_window);
}

const gchar* tips_include_get_selected(LanguageTips* self) {
	GtkTreeIter iter;
	GtkTreeSelection* sel;
	const gchar* result = 0;

	sel = gtk_tree_view_get_selection(self->tips_include_view);
	if( gtk_tree_selection_get_selected(sel, &(self->tips_include_model), &iter) )
		gtk_tree_model_get(self->tips_include_model, &iter, 0, &result, -1);

	return result;
}

static void fill_list_store(LanguageTips* self, GSequence* seq) {
	CppElem* elem;
	gint i;
	gint num;
	GtkTreeIter iter;
	GSequenceIter* seq_iter;

	self->tips_list_seq = seq;
	if( !seq )
		return;

	gtk_tree_view_set_model(self->tips_list_view, 0);

	num = g_sequence_get_length(seq);
	if( num > 250)
		num = 250;

	seq_iter = g_sequence_get_begin_iter(seq);

	for( i=0; i<num; ++i ) {
		elem = (CppElem*)g_sequence_get(seq_iter);
		g_print("%p", elem);
		seq_iter = g_sequence_iter_next(seq_iter);

		gtk_list_store_append( GTK_LIST_STORE(self->tips_list_model), &iter );
		gtk_list_store_set( GTK_LIST_STORE(self->tips_list_model), &iter
			, 0, self->icons[elem->type]
			, 1, elem->name->buf
			, 2, elem
			, -1 );
	}

	gtk_tree_view_set_model(self->tips_list_view, self->tips_list_model);
}

static void clear_list_store(LanguageTips* self) {
	gtk_list_store_clear(GTK_LIST_STORE(self->tips_list_model));
	if( self->tips_list_seq ) {
		g_sequence_free(self->tips_list_seq);
		self->tips_list_seq = 0;
	}
}

void tips_list_tip_show(LanguageTips* self, gint x, gint y, GSequence* seq) {
	gint w;
	gint h;
	GtkTreeIter iter;
	GtkTreeSelection* sel;

	clear_list_store(self);

	if( !seq ) {
		gtk_widget_hide(self->tips_list_window);
		return;
	}

	fill_list_store(self, seq);

	if( gtk_tree_model_get_iter_first(self->tips_list_model, &iter) ) {
		sel = gtk_tree_view_get_selection(self->tips_list_view);
		gtk_tree_selection_select_iter(sel, &iter);
	}

	gtk_tree_view_columns_autosize(self->tips_list_view);

	if( tips_decl_is_visible(self) ) {
		w = 0;
		h = 0;
		gtk_window_get_size(GTK_WINDOW(self->tips_list_window), &w, &h);
		if( y >= (h + 16) ) {
			y -= (h + 16);

		} else {
			y += (h + 5);
		}
	}

	gtk_window_move(GTK_WINDOW(self->tips_list_window), x, y);
	gtk_window_resize(GTK_WINDOW(self->tips_list_window), 200, 100);
	gtk_container_resize_children(GTK_CONTAINER(self->tips_list_window));
	gtk_widget_show(self->tips_list_window);
}

void tips_decl_tip_show(LanguageTips* self, gint x, gint y, GSequence* seq) {
	gint i;
	gint num;
	CppElem* elem;
	GSequenceIter* it;
	gchar line_str[64];
	GtkTextIter text_iter;

	if( self->tips_decl_seq )
		g_sequence_free(self->tips_decl_seq);

	self->tips_decl_seq = seq;
	if( !seq ) {
		gtk_widget_hide(self->tips_decl_window);
		return;
	}

	line_str[0] = '\0';
	gtk_text_buffer_set_text(self->tips_decl_buffer, line_str, 0);

	num = g_sequence_get_length(seq);
	it = g_sequence_get_begin_iter(seq);

	for( i=0; i<num; ++i ) {
		elem = (CppElem*)g_sequence_get(it);
		it = g_sequence_iter_next(it);

		gtk_text_buffer_insert_at_cursor(self->tips_decl_buffer, elem->decl->buf, tiny_str_len(elem->decl));
		gtk_text_buffer_insert_at_cursor(self->tips_decl_buffer, "\n    // ", -1);
		gtk_text_buffer_insert_at_cursor(self->tips_decl_buffer, elem->file->filename->buf, tiny_str_len(elem->file->filename));
		g_snprintf(line_str, 64, ":%d\n\n", elem->sline);
		gtk_text_buffer_insert_at_cursor(self->tips_decl_buffer, line_str, -1);
	}

	gtk_text_buffer_get_iter_at_offset(self->tips_decl_buffer, &text_iter, 0);
	gtk_text_buffer_place_cursor(self->tips_decl_buffer, &text_iter);

	gtk_window_move(GTK_WINDOW(self->tips_decl_window), x, y);
	gtk_window_resize(GTK_WINDOW(self->tips_decl_window), 200, 100);
	gtk_container_resize_children(GTK_CONTAINER(self->tips_decl_window));
	gtk_widget_show(self->tips_decl_window);
}

CppElem* tips_list_get_selected(LanguageTips* self) {
	GtkTreeIter iter;
	GtkTreeSelection* sel;
    CppElem* result = 0;

	sel = gtk_tree_view_get_selection(self->tips_list_view);
	if( gtk_tree_selection_get_selected(sel, &(self->tips_list_model), &iter) )
		gtk_tree_model_get(self->tips_list_model, &iter, 2, &result, -1);

	return result;
}

void tips_select_next(LanguageTips* self) {
	GtkTreeSelection* sel;
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeView* view = 0;

	if( tips_list_is_visible(self) )
		view = self->tips_list_view;
	else if( tips_include_is_visible(self) )
		view = self->tips_include_view;

	if( !view )
		return;

	model = gtk_tree_view_get_model(view);
	sel = gtk_tree_view_get_selection(view);

	if( gtk_tree_selection_get_selected(sel, &model, &iter) ) {
		if( !gtk_tree_model_iter_next(model, &iter) )
			return;

	} else if( !gtk_tree_model_iter_children(model, &iter, NULL) ) {
		return;
	}

	gtk_tree_selection_select_iter(sel, &iter);
	gtk_tree_view_scroll_to_cell(view, gtk_tree_model_get_path(model, &iter), NULL, FALSE, 0.0f, 0.0f);
}

void tips_select_prev(LanguageTips* self) {
	gint count;
	GtkTreePath* path;
	GtkTreeSelection* sel;
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeView* view = 0;

	if( tips_list_is_visible(self) )
		view = self->tips_list_view;
	else if( tips_include_is_visible(self) )
		view = self->tips_include_view;

	if( !view )
		return;

	model = gtk_tree_view_get_model(view);
	sel = gtk_tree_view_get_selection(view);

	if( gtk_tree_selection_get_selected(sel, &model, &iter) ) {
		path = gtk_tree_model_get_path(model, &iter);
		if( !gtk_tree_path_prev(path) )
			return;

		if( !gtk_tree_model_get_iter(model, &iter, path) )
			return;

	} else {
		count = gtk_tree_model_iter_n_children(model, &iter);
		if( count <= 0 || !gtk_tree_model_iter_nth_child(model, &iter, NULL, (count-1)) )
			return;
	}

	gtk_tree_selection_select_iter(sel, &iter);
	gtk_tree_view_scroll_to_cell(view, gtk_tree_model_get_path(model, &iter), NULL, FALSE, 0.0f, 0.0f);
}

gboolean tips_locate_sub(LanguageTips* self, gint x, gint y, const gchar* key) {
	CppElem* elem;
	GtkTreeSelection* sel;
	GtkTreePath* path;
	GtkTreeIter iter;

	if( !gtk_tree_model_iter_children(self->tips_list_model, &iter, NULL) )
		return FALSE;

	do {
		gtk_tree_model_get(self->tips_list_model, &iter, 2, &elem, -1);
		assert( elem );

		if( g_str_has_prefix(elem->name->buf, key) ) {
			sel = gtk_tree_view_get_selection(self->tips_list_view);
			gtk_tree_selection_select_iter(sel, &iter);
			path = gtk_tree_model_get_path(self->tips_list_model, &iter);
			gtk_tree_view_scroll_to_cell(self->tips_list_view, path, NULL, FALSE, 0.0f, 0.0f);
			//gtk_widget_trigger_tooltip_query(GTK_WIDGET(self->tips_list_view));	// TODO : no effect, need test
			return TRUE;
		}

	} while( gtk_tree_model_iter_next(self->tips_list_model, &iter) );

	sel = gtk_tree_view_get_selection(self->tips_list_view);
	gtk_tree_selection_unselect_all(sel);
	return FALSE;
}

