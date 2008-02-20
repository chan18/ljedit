// MiniLineModules.cpp
// 

#include "MiniLineModules.h"

#include "MiniLine.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>

#include <memory.h>
#include <stdlib.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"


GdkColor FAILED_COLOR = { 0, 65535, 10000, 10000 };

void get_insert_pos(GtkTextBuffer* buf, gint* line, gint* offset) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	*line = gtk_text_iter_get_line(&iter);
	*offset = gtk_text_iter_get_line_offset(&iter);
}

gboolean get_current_document_insert_pos(Puss* app, gint* line, gint* offset) {
	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view )
		return FALSE;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	get_insert_pos(buf, line, offset);
	return TRUE;
}

gboolean move_cursor_to_pos(GtkTextView* view, gint line, gint offset) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	if( (line < 0 ) || (line >= gtk_text_buffer_get_line_count(buf)) )
		return FALSE;

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line(buf, &iter, line);
	if( offset < gtk_text_iter_get_chars_in_line(&iter) )
		gtk_text_iter_set_line_offset(&iter, offset);
	else
		gtk_text_iter_forward_to_line_end(&iter);

	gtk_text_buffer_place_cursor(buf, &iter);
	gtk_text_view_scroll_to_iter(view, &iter, FALSE, FALSE, 0.25, 0.25);
	return TRUE;
}

//--------------------------------------------------------------
// mini line GOTO
//--------------------------------------------------------------

struct MiniLineGoto {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
};

gboolean GOTO_cb_active(Puss* app, gpointer tag) {
	MiniLineGoto* self = (MiniLineGoto*)tag;

	gtk_widget_modify_base(GTK_WIDGET(app->mini_line->entry), GTK_STATE_NORMAL, NULL);

	if( get_current_document_insert_pos(app, &(self->last_line), &(self->last_offset)) ) {
		gchar text[64] = { 0 };
		
#ifdef WIN32
		sprintf_s(text, "%d", (self->last_line + 1));
#else
		snprintf(text, 64, "%d", (self->last_line + 1));
#endif

		gtk_label_set_text(app->mini_line->label, _("goto :"));
		gtk_entry_set_text(app->mini_line->entry, text);
		gtk_entry_select_region(app->mini_line->entry, 0, -1);

		return TRUE;
	}	
	return FALSE;
}

void GOTO_cb_changed(Puss* app, gpointer tag) {
	MiniLineGoto* self = (MiniLineGoto*)tag;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view ) {
		puss_mini_line_deactive(app);
		return;
	}

	const gchar* text = gtk_entry_get_text(app->mini_line->entry);
	gint line = atoi(text) - 1;

	gboolean res = move_cursor_to_pos(view, line, self->last_offset);
	gtk_widget_modify_base(GTK_WIDGET(app->mini_line->entry), GTK_STATE_NORMAL, res ? NULL : &FAILED_COLOR);
}

gboolean GOTO_cb_key_press(Puss* app, GdkEventKey* event, gpointer tag) {
	MiniLineGoto* self = (MiniLineGoto*)tag;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view ) {
		puss_mini_line_deactive(app);
		return TRUE;
	}

	switch( event->keyval ) {
	case GDK_Escape:
		move_cursor_to_pos(view, self->last_line, self->last_offset);
		puss_mini_line_deactive(app);
		return TRUE;

	case GDK_Return:
		//doc_mgr_.pos_add(*current_page_, last.get_line(), last.get_line_offset());
		//doc_mgr_.pos_add(*current_page_, iter.get_line(), iter.get_line_offset());
		puss_mini_line_deactive(app);
		return TRUE;
	}

	return FALSE;
}

MiniLineCallback* puss_mini_line_GOTO_get_callback() {
	static MiniLineGoto me;
	me.cb.tag = &me;
	me.cb.cb_active = &GOTO_cb_active;
	me.cb.cb_changed = &GOTO_cb_changed;
	me.cb.cb_key_press = &GOTO_cb_key_press;

	return &me.cb;
}

//--------------------------------------------------------------
// mini line FIND
//--------------------------------------------------------------

struct MiniLineFind {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
};

gboolean find_next_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	GtkTextIter iter, end;
	gtk_text_buffer_get_selection_bounds(buf, &iter, &end);
	if( skip_current )
		gtk_text_iter_forward_char(&iter);
	gtk_text_buffer_get_end_iter(buf, &end);

	GtkSourceSearchFlags flag = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	if( !gtk_source_iter_forward_search(&iter, text, flag, ps, pe, &end) )
	{
		gtk_text_buffer_get_start_iter(buf, &iter);

		if( !gtk_source_iter_forward_search(&iter, text, flag, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

gboolean find_prev_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean /*skip_current*/) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	GtkTextIter iter, end;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	gtk_text_buffer_get_start_iter(buf, &end);

	GtkSourceSearchFlags flag = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	if( !gtk_source_iter_backward_search(&iter, text, flag, ps, pe, &end) )
	{
		gtk_text_buffer_get_end_iter(buf, &iter);

		if( !gtk_source_iter_backward_search(&iter, text, flag, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

void find_and_locate_text(Puss* app, GtkTextView* view, const gchar* text, gboolean is_forward, gboolean skip_current) {
	GtkTextIter ps, pe;
	gboolean res = is_forward
		? find_next_text(view, text, &ps, &pe, skip_current)
		: find_prev_text(view, text, &ps, &pe, skip_current);

	if( res ) {
		gtk_widget_modify_base(GTK_WIDGET(app->mini_line->entry), GTK_STATE_NORMAL, NULL);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(view), &ps, &pe);
		gtk_text_view_scroll_to_iter(view, &ps, FALSE, FALSE, 0.25, 0.25);
	} else {
		gtk_widget_modify_base(GTK_WIDGET(app->mini_line->entry), GTK_STATE_NORMAL, &FAILED_COLOR);
	}
}

struct MiniLineFIND {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
};

gboolean FIND_cb_active(Puss* app, gpointer tag) {
	MiniLineFIND* self = (MiniLineFIND*)tag;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view )
		return FALSE;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	get_insert_pos(buf, &(self->last_line), &(self->last_offset));

	gtk_widget_modify_base(GTK_WIDGET(app->mini_line->entry), GTK_STATE_NORMAL, NULL);

	gtk_label_set_text(app->mini_line->label, _("find :"));

	GtkTextIter ps, pe;
	gtk_text_buffer_get_selection_bounds(buf, &ps, &pe);
	if( !gtk_text_iter_equal(&ps, &pe) ) {
		gchar* text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		gtk_entry_set_text(app->mini_line->entry, text);
		g_free(text);
	}
	gtk_entry_select_region(app->mini_line->entry, 0, -1);

	return TRUE;
}

void FIND_cb_changed(Puss* app, gpointer tag) {
	//MiniLineFIND* self = (MiniLineFIND*)tag;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view ) {
		puss_mini_line_deactive(app);
		return;
	}

	const gchar* text = gtk_entry_get_text(app->mini_line->entry);
	if( !text || *text=='\0' )
		return;

	find_and_locate_text(app, view, text, TRUE, FALSE);
}

gboolean FIND_cb_key_press(Puss* app, GdkEventKey* event, gpointer tag) {
	MiniLineFIND* self = (MiniLineFIND*)tag;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view ) {
		puss_mini_line_deactive(app);
		return TRUE;
	}

	const gchar* text = gtk_entry_get_text(app->mini_line->entry);
	if( !text || *text=='\0' )
		return FALSE;

	switch( event->keyval ) {
	case GDK_Escape:
		move_cursor_to_pos(view, self->last_line, self->last_offset);
		puss_mini_line_deactive(app);
		return TRUE;

	case GDK_Return:
		//doc_mgr_.pos_add(*current_page_, last.get_line(), last.get_line_offset());
		//doc_mgr_.pos_add(*current_page_, iter.get_line(), iter.get_line_offset());
		puss_mini_line_deactive(app);
		return TRUE;

	case GDK_Up:
		find_and_locate_text(app, view, text, FALSE, TRUE);
		return TRUE;

	case GDK_Down:
		find_and_locate_text(app, view, text, TRUE, TRUE);
		return TRUE;
	}

	return FALSE;
}

MiniLineCallback* puss_mini_line_FIND_get_callback() {
	static MiniLineFind me;
	me.cb.tag = &me;
	me.cb.cb_active = &FIND_cb_active;
	me.cb.cb_changed = &FIND_cb_changed;
	me.cb.cb_key_press = &FIND_cb_key_press;

	return &me.cb;
}

// TODO : 

//--------------------------------------------------------------
// mini line REPLACE
//--------------------------------------------------------------


MiniLineCallback* puss_mini_line_REPLACE_get_callback() {
	return puss_mini_line_FIND_get_callback();
}

