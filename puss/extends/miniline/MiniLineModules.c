// MiniLineModules.c
// 

#include "IMiniLine.h"

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>

#include <memory.h>
#include <stdlib.h>
#include <string.h>


GdkColor FAILED_COLOR = { 0, 65535, 10000, 10000 };

static void get_insert_pos(GtkTextBuffer* buf, gint* line, gint* offset) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	*line = gtk_text_iter_get_line(&iter);
	*offset = gtk_text_iter_get_line_offset(&iter);
}

static gboolean get_current_document_insert_pos(MiniLine* miniline, gint* line, gint* offset) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	GtkTextBuffer* buf;
	GtkTextView* view;

	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	get_insert_pos(buf, line, offset);
	return TRUE;
}

static gboolean move_cursor_to_pos(GtkTextView* view, gint line, gint offset) {
	GtkTextIter iter;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	if( (line < 0 ) || (line >= gtk_text_buffer_get_line_count(buf)) )
		return FALSE;

	gtk_text_buffer_get_iter_at_line(buf, &iter, line);

	if( offset < gtk_text_iter_get_bytes_in_line(&iter) )
		gtk_text_iter_set_line_offset(&iter, offset);
	else if( !gtk_text_iter_ends_line(&iter) )
		gtk_text_iter_forward_to_line_end(&iter);

	gtk_text_buffer_place_cursor(buf, &iter);
	gtk_text_view_scroll_to_iter(view, &iter, 0.0, FALSE, 1.0, 0.25);
	return TRUE;
}

typedef struct _MiniLineFind {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
} MiniLineFind;

static gboolean find_next_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current) {
	GtkTextIter iter, end;
	GtkSourceSearchFlags flags = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_selection_bounds(buf, &iter, &end);
	if( skip_current )
		gtk_text_iter_forward_char(&iter);
	gtk_text_buffer_get_end_iter(buf, &end);

	if( !gtk_source_iter_forward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_start_iter(buf, &iter);

		if( !gtk_source_iter_forward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static gboolean find_prev_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current) {
	GtkTextIter iter, end;
	GtkSourceSearchFlags flags = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	gtk_text_buffer_get_start_iter(buf, &end);

	if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_end_iter(buf, &iter);

		if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static void find_and_locate_text(MiniLine* miniline, GtkTextView* view, const gchar* text, gboolean is_forward, gboolean skip_current) {
	GtkTextIter ps, pe;
	gboolean res = is_forward
		? find_next_text(view, text, &ps, &pe, skip_current)
		: find_prev_text(view, text, &ps, &pe, skip_current);

	if( res ) {
		gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(view), &ps, &pe);
		gtk_text_view_scroll_to_iter(view, &ps, 0.0, FALSE, 1.0, 0.25);
	} else {
		gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, &FAILED_COLOR);
	}
}

static void fix_selected_range(GtkTextView* view) {
	GtkTextIter ps, pe;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( buf && gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) )
		gtk_text_buffer_select_range(buf, &pe, &ps);
}

//--------------------------------------------------------------
// mini line GOTO
//--------------------------------------------------------------

typedef struct _MiniLineGoto {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
} MiniLineGoto;

static gboolean GOTO_cb_active(MiniLine* miniline, gpointer tag) {
	gchar text[64] = { 0 };

	MiniLineGoto* self = (MiniLineGoto*)tag;

	gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);

	if( get_current_document_insert_pos(miniline, &(self->last_line), &(self->last_offset)) ) {
		g_snprintf( text, 64, "%d", (self->last_line + 1) );

		gtk_image_set_from_stock(miniline->image, GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_entry_set_text(miniline->entry, text);
		gtk_entry_select_region(miniline->entry, 0, -1);

		return TRUE;
	}	
	return FALSE;
}

static void GOTO_cb_changed(MiniLine* miniline, gpointer tag) {
	const gchar* text;
	gint line;
	gboolean res;
	gint page_num;
	GtkTextView* view;

	MiniLineGoto* self = (MiniLineGoto*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return;
	}

	text = gtk_entry_get_text(miniline->entry);
	line = atoi(text) - 1;
	res = move_cursor_to_pos(view
		, (line < 0) ? self->last_line : line
		, self->last_offset);
	gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, res ? NULL : &FAILED_COLOR);
}

static gboolean GOTO_cb_key_press(MiniLine* miniline, GdkEventKey* event, gpointer tag) {
	gint page_num;
	GtkTextView* view;
	MiniLineGoto* self = (MiniLineGoto*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return TRUE;
	}

	switch( event->keyval ) {
	case GDK_Escape:
		move_cursor_to_pos(view, self->last_line, self->last_offset);
		miniline->deactive(miniline);
		return TRUE;

	case GDK_Return:
		miniline->deactive(miniline);
		return TRUE;
	}

	return FALSE;
}

PUSS_EXPORT MiniLineCallback* miniline_GOTO_get_callback() {
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

typedef struct _MiniLineFIND {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
} MiniLineFIND;

static gboolean FIND_cb_active(MiniLine* miniline, gpointer tag) {
	GtkTextBuffer* buf;
	GtkTextIter ps, pe;
	gchar* text;
	gint page_num;
	GtkTextView* view;

	MiniLineFIND* self = (MiniLineFIND*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	get_insert_pos(buf, &(self->last_line), &(self->last_offset));

	gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);

	gtk_image_set_from_stock(miniline->image, GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);

	if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
		text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		gtk_entry_set_text(miniline->entry, text);
		g_free(text);
	}
	gtk_entry_select_region(miniline->entry, 0, -1);

	return TRUE;
}

static void FIND_cb_changed(MiniLine* miniline, gpointer tag) {
	const gchar* text;
	gint page_num;
	GtkTextView* view;

	MiniLineFIND* self = (MiniLineFIND*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return;
	}

	text = gtk_entry_get_text(miniline->entry);
	if( *text=='\0' )
		move_cursor_to_pos(view, self->last_line, self->last_offset);
	else
		find_and_locate_text(miniline, view, text, TRUE, FALSE);
}

static gboolean FIND_cb_key_press(MiniLine* miniline, GdkEventKey* event, gpointer tag) {
	const gchar* text;
	gint page_num;
	GtkTextView* view;

	MiniLineFIND* self = (MiniLineFIND*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return TRUE;
	}

	if( event->keyval==GDK_Escape ) {
		move_cursor_to_pos(view, self->last_line, self->last_offset);
		miniline->deactive(miniline);
		return TRUE;
	}

	text = gtk_entry_get_text(miniline->entry);
	if( *text=='\0' )
		return FALSE;

	switch( event->keyval ) {
	case GDK_Return:
		miniline->deactive(miniline);
		fix_selected_range(view);
		return TRUE;

	case GDK_Up:
		find_and_locate_text(miniline, view, text, FALSE, TRUE);
		return TRUE;

	case GDK_Down:
		find_and_locate_text(miniline, view, text, TRUE, TRUE);
		return TRUE;
	}

	return FALSE;
}

PUSS_EXPORT MiniLineCallback* miniline_FIND_get_callback() {
	static MiniLineFind me;
	me.cb.tag = &me;
	me.cb.cb_active = &FIND_cb_active;
	me.cb.cb_changed = &FIND_cb_changed;
	me.cb.cb_key_press = &FIND_cb_key_press;

	return &me.cb;
}

//--------------------------------------------------------------
// mini line REPLACE
//--------------------------------------------------------------

typedef struct _MiniLineREPLACE {
	MiniLineCallback cb;
} MiniLineREPLACE;

static gchar* get_replace_search_text(GtkEntry* entry) {
	gchar* p;
	gchar* text = g_strdup(gtk_entry_get_text(entry));
	for( p = text; *p ; ++p ) {
		if( *p=='/' ) {
			*p = '\0';
			break;
		}
	}
	return text;
}

static gboolean REPLACE_cb_active(MiniLine* miniline, gpointer tag) {
	GtkTextBuffer* buf;
	gchar* text = 0;
	gchar* stext;
	gint page_num;
	GtkTextView* view;
	gint sel_ps = 0;
	GtkTextIter ps, pe;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);

	gtk_image_set_from_stock(miniline->image, GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_SMALL_TOOLBAR);

	if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
		text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		sel_ps = (gint)strlen(text) + 1;
	} else {
		text = g_strdup("<text>");
	}

	stext = g_strconcat(text, "/<replace>/[all]", NULL);
	gtk_entry_set_text(miniline->entry, stext);
	gtk_entry_select_region(miniline->entry, sel_ps, -1);

	g_free(text);
	g_free(stext);

	return TRUE;
}

static void REPLACE_cb_changed(MiniLine* miniline, gpointer tag) {
	gchar* text;
	gint page_num;
	GtkTextView* view;

	//MiniLineREPLACE* self = (MiniLineREPLACE*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return;
	}

	text = get_replace_search_text(miniline->entry);

	if( *text!='\0' )
		find_and_locate_text(miniline, view, text, TRUE, FALSE);

	g_free(text);
}

static gchar* locate_next_scope(gchar* text) {
	gchar* p;
	gchar* pos = 0;
	for( p = text; *p; ++p ) {
		if( *p=='/' ) {
			*p = '\0';
			pos = p + 1;
			break;
		}
	}
	return pos;
}

static void do_replace_all( GtkTextBuffer* buf
		, const gchar* find_text
		, const gchar* replace_text
		, gint flags )
{
	gint replace_text_len;
	GtkTextIter iter, ps, pe;
	gtk_text_buffer_get_start_iter(buf, &iter);

	gtk_text_buffer_begin_user_action(buf);
	{
		replace_text_len = (gint)strlen(replace_text);

		while( gtk_source_iter_forward_search(&iter
				, find_text
				, (GtkSourceSearchFlags)flags
				, &ps
				, &pe
				, 0) )
		{
			gtk_text_buffer_delete(buf, &ps, &pe);
			gtk_text_buffer_insert(buf, &ps, replace_text, replace_text_len);
			iter = ps;
		}
	}
	gtk_text_buffer_end_user_action(buf);
}

static void replace_and_locate_text(MiniLine* miniline, GtkTextView* view) {
	gchar* text;
	gchar* ops_text;
	gboolean replace_all_sign;
	GtkTextBuffer* buf;
	GtkTextIter ps, pe;

	gchar* find_text = g_strdup(gtk_entry_get_text(miniline->entry));
	gchar* replace_text = locate_next_scope(find_text);

	if( !replace_text ) {
		text = g_strconcat(find_text, "/<replace>/[all]", NULL);
		gtk_entry_set_text(miniline->entry, text);
		g_free(text);

		gtk_entry_select_region(miniline->entry, (gint)strlen(find_text), -1);

	} else {
		ops_text = locate_next_scope(replace_text);
		replace_all_sign = (ops_text && ops_text[0]=='a');

		buf = gtk_text_view_get_buffer(view);

		if( replace_all_sign ) {
			do_replace_all( buf
								, find_text
								, replace_text
								, GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE );

			miniline->deactive(miniline);

		} else {
			gtk_text_buffer_get_selection_bounds(buf, &ps, &pe);
			if( gtk_text_iter_is_end(&ps) || gtk_text_iter_is_end(&pe) ) {
				miniline->deactive(miniline);
				
			} else {
				gtk_text_buffer_begin_user_action(buf);
				gtk_text_buffer_delete(buf, &ps, &pe);
				gtk_text_buffer_insert(buf, &ps, replace_text, (gint)strlen(replace_text));
				gtk_text_buffer_end_user_action(buf);

				find_and_locate_text(miniline, view, find_text, TRUE, FALSE);
			}
		}
	}

	g_free(find_text);
}

static gboolean REPLACE_cb_key_press(MiniLine* miniline, GdkEventKey* event, gpointer tag) {
	gint page_num;
	GtkTextView* view;
	const gchar* text;
	gchar* rtext;

	if( event->keyval==GDK_Escape ) {
		miniline->deactive(miniline);
		return TRUE;
	}

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return TRUE;
	}

	text = gtk_entry_get_text(miniline->entry);
	if( *text=='\0' || *text=='/' )
		return FALSE;

	if( *text!='\0' ) {
		switch( event->keyval ) {
		case GDK_Return:
			replace_and_locate_text(miniline, view);
			return TRUE;

		case GDK_Up:
		case GDK_Down:
			{
				rtext = get_replace_search_text(miniline->entry);
				find_and_locate_text(miniline, view, rtext, event->keyval==GDK_Down, TRUE);
				g_free(rtext);
			}
			return TRUE;
		}
	}

	return FALSE;
}

PUSS_EXPORT MiniLineCallback* miniline_REPLACE_get_callback() {
	static MiniLineREPLACE me;
	me.cb.tag = &me;
	me.cb.cb_active = &REPLACE_cb_active;
	me.cb.cb_changed = &REPLACE_cb_changed;
	me.cb.cb_key_press = &REPLACE_cb_key_press;

	return &me.cb;
}

