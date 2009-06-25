// Utils.c
// 

#include "LanguageTips.h"

static void search_elem_locate(GtkTextBuffer* buf, gint* pline, gint* poffset, CppElem* elem) {
	GtkTextIter iter;
	GtkTextIter limit;
	GtkTextIter ps;
	GtkTextIter pe;

	*pline = elem->sline - 1;
	*poffset = -1;

	gtk_text_buffer_get_iter_at_line(buf, &iter, *pline);
	limit = iter;
	gtk_text_iter_forward_to_line_end(&limit);

	if( gtk_text_iter_forward_search(&iter, elem->name->buf, 0, &ps, &pe, &limit) )
		*poffset = gtk_text_iter_get_line_offset(&ps);
}

gboolean open_and_locate_elem(LanguageTips* self, CppElem* elem) {
	if( !elem )
		return FALSE;

	if( !self->app->doc_open_locate(elem->file->filename->buf, search_elem_locate, elem, FALSE) )
		return FALSE;

	return TRUE;
}

