// MiniLineModules.cpp
// 

#include "MiniLineModules.h"

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "Puss.h"
#include "MiniLine.h"
#include "DocManager.h"
#include "Utils.h"


GdkColor FAILED_COLOR = { 0, 65535, 10000, 10000 };

void get_insert_pos(GtkTextBuffer* buf, gint* line, gint* offset) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	*line = gtk_text_iter_get_line(&iter);
	*offset = gtk_text_iter_get_line_offset(&iter);
}

gboolean get_current_document_insert_pos(gint* line, gint* offset) {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
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

	if( offset < gtk_text_iter_get_bytes_in_line(&iter) )
		gtk_text_iter_set_line_offset(&iter, offset);
	else if( !gtk_text_iter_ends_line(&iter) )
		gtk_text_iter_forward_to_line_end(&iter);

	gtk_text_buffer_place_cursor(buf, &iter);
	gtk_text_view_scroll_to_iter(view, &iter, 0.0, FALSE, 1.0, 0.25);
	return TRUE;
}

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

	GtkSourceSearchFlags flags = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	if( !gtk_source_iter_forward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_start_iter(buf, &iter);

		if( !gtk_source_iter_forward_search(&iter, text, flags, ps, pe, &end) )
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

	GtkSourceSearchFlags flags = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_end_iter(buf, &iter);

		if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

void find_and_locate_text(GtkTextView* view, const gchar* text, gboolean is_forward, gboolean skip_current) {
	GtkTextIter ps, pe;
	gboolean res = is_forward
		? find_next_text(view, text, &ps, &pe, skip_current)
		: find_prev_text(view, text, &ps, &pe, skip_current);

	if( res ) {
		gtk_widget_modify_base(GTK_WIDGET(puss_app->mini_line->entry), GTK_STATE_NORMAL, NULL);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(view), &ps, &pe);
		gtk_text_view_scroll_to_iter(view, &ps, 0.0, FALSE, 1.0, 0.25);
	} else {
		gtk_widget_modify_base(GTK_WIDGET(puss_app->mini_line->entry), GTK_STATE_NORMAL, &FAILED_COLOR);
	}
}

void fix_selected_range(GtkTextView* view) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( buf ) {
		GtkTextIter ps, pe;
		if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) )
			gtk_text_buffer_select_range(buf, &pe, &ps);
	}
}

//--------------------------------------------------------------
// mini line GOTO
//--------------------------------------------------------------

struct MiniLineGoto {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
};

gboolean GOTO_cb_active(gpointer tag) {
	MiniLineGoto* self = (MiniLineGoto*)tag;

	gtk_widget_modify_base(GTK_WIDGET(puss_app->mini_line->entry), GTK_STATE_NORMAL, NULL);

	if( get_current_document_insert_pos(&(self->last_line), &(self->last_offset)) ) {
		gchar text[64] = { 0 };
		g_snprintf( text, 64, "%d", (self->last_line + 1) );

		gtk_image_set_from_stock(puss_app->mini_line->image, GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_entry_set_text(puss_app->mini_line->entry, text);
		gtk_entry_select_region(puss_app->mini_line->entry, 0, -1);

		return TRUE;
	}	
	return FALSE;
}

void GOTO_cb_changed(gpointer tag) {
	MiniLineGoto* self = (MiniLineGoto*)tag;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view ) {
		puss_mini_line_deactive();
		return;
	}

	const gchar* text = gtk_entry_get_text(puss_app->mini_line->entry);
	gint line = atoi(text) - 1;
	gboolean res = move_cursor_to_pos(view
		, (line < 0) ? self->last_line : line
		, self->last_offset);
	gtk_widget_modify_base(GTK_WIDGET(puss_app->mini_line->entry), GTK_STATE_NORMAL, res ? NULL : &FAILED_COLOR);
}

gboolean GOTO_cb_key_press(GdkEventKey* event, gpointer tag) {
	MiniLineGoto* self = (MiniLineGoto*)tag;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view ) {
		puss_mini_line_deactive();
		return TRUE;
	}

	switch( event->keyval ) {
	case GDK_Escape:
		move_cursor_to_pos(view, self->last_line, self->last_offset);
		puss_mini_line_deactive();
		return TRUE;

	case GDK_Return:
		//doc_mgr_.pos_add(*current_page_, last.get_line(), last.get_line_offset());
		//doc_mgr_.pos_add(*current_page_, iter.get_line(), iter.get_line_offset());
		puss_mini_line_deactive();
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

struct MiniLineFIND {
	gint	last_line;
	gint	last_offset;

	MiniLineCallback cb;
};

gboolean FIND_cb_active(gpointer tag) {
	MiniLineFIND* self = (MiniLineFIND*)tag;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	get_insert_pos(buf, &(self->last_line), &(self->last_offset));

	gtk_widget_modify_base(GTK_WIDGET(puss_app->mini_line->entry), GTK_STATE_NORMAL, NULL);

	gtk_image_set_from_stock(puss_app->mini_line->image, GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);

	GtkTextIter ps, pe;
	if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
		gchar* text = 0;
		text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		gtk_entry_set_text(puss_app->mini_line->entry, text);
		g_free(text);
	}
	gtk_entry_select_region(puss_app->mini_line->entry, 0, -1);

	return TRUE;
}

void FIND_cb_changed(gpointer tag) {
	MiniLineFIND* self = (MiniLineFIND*)tag;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view ) {
		puss_mini_line_deactive();
		return;
	}

	const gchar* text = gtk_entry_get_text(puss_app->mini_line->entry);
	if( *text=='\0' )
		move_cursor_to_pos(view, self->last_line, self->last_offset);
	else
		find_and_locate_text(view, text, TRUE, FALSE);
}

gboolean FIND_cb_key_press(GdkEventKey* event, gpointer tag) {
	MiniLineFIND* self = (MiniLineFIND*)tag;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view ) {
		puss_mini_line_deactive();
		return TRUE;
	}

	if( event->keyval==GDK_Escape ) {
		move_cursor_to_pos(view, self->last_line, self->last_offset);
		puss_mini_line_deactive();
		return TRUE;
	}

	const gchar* text = gtk_entry_get_text(puss_app->mini_line->entry);
	if( *text=='\0' )
		return FALSE;

	switch( event->keyval ) {
	case GDK_Return:
		//doc_mgr_.pos_add(*current_page_, last.get_line(), last.get_line_offset());
		//doc_mgr_.pos_add(*current_page_, iter.get_line(), iter.get_line_offset());
		puss_mini_line_deactive();
		fix_selected_range(view);
		return TRUE;

	case GDK_Up:
		find_and_locate_text(view, text, FALSE, TRUE);
		return TRUE;

	case GDK_Down:
		find_and_locate_text(view, text, TRUE, TRUE);
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

//--------------------------------------------------------------
// mini line REPLACE
//--------------------------------------------------------------

struct MiniLineREPLACE {
	MiniLineCallback cb;
};

gchar* get_replace_search_text(GtkEntry* entry) {
	gchar* text = g_strdup(gtk_entry_get_text(entry));
	for( gchar* p = text; *p ; ++p ) {
		if( *p=='/' ) {
			*p = '\0';
			break;
		}
	}
	return text;
}

gboolean REPLACE_cb_active(gpointer tag) {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_widget_modify_base(GTK_WIDGET(puss_app->mini_line->entry), GTK_STATE_NORMAL, NULL);

	gtk_image_set_from_stock(puss_app->mini_line->image, GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_SMALL_TOOLBAR);

	gchar* text = 0;
	gint sel_ps = 0;
	GtkTextIter ps, pe;
	if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
		text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		sel_ps = (gint)strlen(text) + 1;
	} else {
		text = g_strdup("<text>");
	}

	gchar* stext = g_strconcat(text, "/<replace>/[all]", NULL);
	gtk_entry_set_text(puss_app->mini_line->entry, stext);
	gtk_entry_select_region(puss_app->mini_line->entry, sel_ps, -1);

	g_free(text);
	g_free(stext);

	return TRUE;
}

void REPLACE_cb_changed(gpointer tag) {
	MiniLineREPLACE* self = (MiniLineREPLACE*)tag;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view ) {
		puss_mini_line_deactive();
		return;
	}

	gchar* text = get_replace_search_text(puss_app->mini_line->entry);

	if( *text!='\0' )
		find_and_locate_text(view, text, TRUE, FALSE);

	g_free(text);
}

gchar* locate_next_scope(gchar* text) {
	gchar* pos = 0;
	for( gchar* p = text; *p; ++p ) {
		if( *p=='/' ) {
			*p = '\0';
			pos = p + 1;
			break;
		}
	}
	return pos;
}

void replace_and_locate_text(GtkTextView* view) {
	gchar* find_text = g_strdup(gtk_entry_get_text(puss_app->mini_line->entry));

	gchar* replace_text = locate_next_scope(find_text);

	if( !replace_text ) {
		gchar* text = g_strconcat(find_text, "/<replace>/[all]", NULL);
		gtk_entry_set_text(puss_app->mini_line->entry, text);
		g_free(text);

		gtk_entry_select_region(puss_app->mini_line->entry, (gint)strlen(find_text), -1);

	} else {
		gchar* ops_text = locate_next_scope(replace_text);
		gboolean replace_all_sign = (ops_text && ops_text[0]=='a');

		GtkTextBuffer* buf = gtk_text_view_get_buffer(view);

		if( replace_all_sign ) {
			puss_doc_replace_all( buf
								, find_text
								, replace_text
								, GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE );

			puss_mini_line_deactive();

		} else {
			GtkTextIter ps, pe;
			gtk_text_buffer_get_selection_bounds(buf, &ps, &pe);
			if( gtk_text_iter_is_end(&ps) || gtk_text_iter_is_end(&pe) ) {
				puss_mini_line_deactive();
				
			} else {
				gtk_text_buffer_begin_user_action(buf);
				gtk_text_buffer_delete(buf, &ps, &pe);
				gtk_text_buffer_insert(buf, &ps, replace_text, (gint)strlen(replace_text));
				gtk_text_buffer_end_user_action(buf);

				find_and_locate_text(view, find_text, TRUE, FALSE);
			}
		}
	}

	g_free(find_text);
}

gboolean REPLACE_cb_key_press(GdkEventKey* event, gpointer tag) {
	if( event->keyval==GDK_Escape ) {
		puss_mini_line_deactive();
		return TRUE;
	}

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view ) {
		puss_mini_line_deactive();
		return TRUE;
	}

	const gchar* text = gtk_entry_get_text(puss_app->mini_line->entry);
	if( *text=='\0' || *text=='/' )
		return FALSE;

	if( *text!='\0' ) {
		switch( event->keyval ) {
		case GDK_Return:
			replace_and_locate_text(view);
			return TRUE;

		case GDK_Up:
		case GDK_Down:
			{
				gchar* text = get_replace_search_text(puss_app->mini_line->entry);
				find_and_locate_text(view, text, event->keyval==GDK_Down, TRUE);
				g_free(text);
			}
			return TRUE;
		}
	}

	return FALSE;
}

MiniLineCallback* puss_mini_line_REPLACE_get_callback() {
	static MiniLineREPLACE me;
	me.cb.tag = &me;
	me.cb.cb_active = &REPLACE_cb_active;
	me.cb.cb_changed = &REPLACE_cb_changed;
	me.cb.cb_key_press = &REPLACE_cb_key_press;

	return &me.cb;
}

