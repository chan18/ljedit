// Utils.c
// 

#include "LanguageTips.h"

static gboolean search_elem_locate(GtkTextBuffer* buf, gint* pline, gint* poffset, CppElem* elem) {
	GtkTextIter iter;
	GtkTextIter limit;
	GtkTextIter ps;
	GtkTextIter pe;
	guint ch;
	gboolean full_matched;
	gboolean need_move_cursor;

	*pline = elem->sline - 1;
	*poffset = -1;

	gtk_text_buffer_get_iter_at_line(buf, &iter, *pline);
	limit = iter;
	gtk_text_iter_forward_to_line_end(&limit);

	need_move_cursor = TRUE;
	full_matched = FALSE;
	while( gtk_text_iter_forward_search(&iter, elem->name->buf, 0, &ps, &pe, &limit) ) {
		*poffset = gtk_text_iter_get_line_offset(&pe);
		gtk_text_buffer_select_range(buf, &pe, &ps);
		need_move_cursor = FALSE;

		if( gtk_text_iter_starts_line(&ps) ) {
			full_matched = TRUE;
		} else {
			iter = ps;
			gtk_text_iter_backward_char(&iter);
			ch = gtk_text_iter_get_char(&iter);
			if( !g_unichar_isalnum(ch) && ch!='_' )
				full_matched = TRUE;
		}

		if( full_matched && !gtk_text_iter_ends_line(&pe) ) {
			iter = pe;
			gtk_text_iter_forward_char(&iter);
			ch = gtk_text_iter_get_char(&iter);
			if( g_unichar_isalnum(ch) || ch=='_' )
				full_matched = FALSE;
		}

		if( full_matched )
			break;

		iter = ps;
		gtk_text_iter_forward_char(&iter);
	}

	return need_move_cursor;
}

gboolean open_and_locate_elem(LanguageTips* self, CppElem* elem) {
	if( !elem )
		return FALSE;

	if( !self->app->doc_open_locate(elem->file->filename->buf, (FindLocation)search_elem_locate, elem, FALSE) )
		return FALSE;

	return TRUE;
}

