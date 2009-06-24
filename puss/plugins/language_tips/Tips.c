// Tips.c
// 

#include "LanguageTips.h"

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
	g_sequence_free(self->tips_list_seq);
	self->tips_list_seq = 0;
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

