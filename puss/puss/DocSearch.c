// DocSearch.c
// 

#include "DocSearch.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>

static gchar* g_last_search_text = 0;

static gboolean find_next_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current, GtkSourceSearchFlags flags) {
	GtkTextIter iter, end;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_mark(buf, "puss:searched_mark_start"));
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

static gboolean find_prev_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current, GtkSourceSearchFlags flags) {
	GtkTextIter iter, end;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_mark(buf, "puss:searched_mark_start"));
	gtk_text_buffer_get_start_iter(buf, &end);

	if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_end_iter(buf, &iter);

		if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static void unmark_all_search_text_matched(GtkTextBuffer* buf, int search_flags) {
	GtkTextIter ps, pe, end;
	if( g_last_search_text ) {
		gtk_text_buffer_get_start_iter(buf, &ps);
		gtk_text_buffer_get_end_iter(buf, &end);

		while( gtk_source_iter_forward_search(&ps, g_last_search_text, search_flags, &ps, &pe, &end) ) {
			gtk_text_buffer_remove_tag_by_name(buf, "puss:searched", &ps, &pe);
			ps = pe;
		}

		g_free(g_last_search_text);
		g_last_search_text = 0;
	}
}

static void mark_all_search_text_matched(GtkTextBuffer* buf, const gchar* text, int search_flags) {
	GtkTextIter ps, pe, end;
	if( g_last_search_text ) {
		if( !text || !g_str_equal(text, g_last_search_text) )
			unmark_all_search_text_matched(buf, search_flags);
	}

	if( text && *text!='\0' ) {
		g_last_search_text = g_strdup(text);
		if( g_last_search_text ) {
			gtk_text_buffer_get_start_iter(buf, &ps);
			gtk_text_buffer_get_end_iter(buf, &end);

			while( gtk_source_iter_forward_search(&ps, g_last_search_text, search_flags, &ps, &pe, &end) ) {
				gtk_text_buffer_apply_tag_by_name(buf, "puss:searched", &ps, &pe);
				ps = pe;
			}
		}
	}
}

gboolean puss_find_and_locate_text( GtkTextView* view
			, const gchar* text
			, gboolean is_forward
			, gboolean skip_current
			, gboolean mark_current
			, gboolean mark_all
			, gboolean is_continue
			, int search_flags )
{
	GtkTextBuffer* buf;
	GtkTextIter ps, pe;
	gboolean res = FALSE;

	buf = gtk_text_view_get_buffer(view);
	if( mark_current && g_last_search_text ) {
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_mark(buf, "puss:searched_mark_start"));
		gtk_text_buffer_get_iter_at_mark(buf, &pe, gtk_text_buffer_get_mark(buf, "puss:searched_mark_end"));
		gtk_text_buffer_remove_tag_by_name(buf, "puss:searched_current", &ps, &pe);
	}

	if( !is_continue ) {
		gtk_text_buffer_get_selection_bounds(buf, &ps, &pe);
		gtk_text_buffer_move_mark_by_name(buf, "puss:searched_mark_start", &ps);
		gtk_text_buffer_move_mark_by_name(buf, "puss:searched_mark_end", &pe);
	}

	if( text ) {
		res = is_forward
			? find_next_text(view, text, &ps, &pe, skip_current, search_flags)
			: find_prev_text(view, text, &ps, &pe, skip_current, search_flags);

		if( res ) {
			gtk_text_view_scroll_to_iter(view, &ps, 0.0, FALSE, 1.0, 0.25);
			if( mark_all )
				mark_all_search_text_matched(buf, text, search_flags);

			gtk_text_buffer_move_mark_by_name(buf, "puss:searched_mark_start", &ps);
			gtk_text_buffer_move_mark_by_name(buf, "puss:searched_mark_end", &pe);
			gtk_text_buffer_place_cursor(buf, &ps);
			if( mark_current )
				gtk_text_buffer_apply_tag_by_name(buf, "puss:searched_current", &ps, &pe);
		}
	}

	if( !res && mark_all )
		unmark_all_search_text_matched(buf, search_flags);

	return res;
}

void puss_show_find_dialog() {
	gtk_widget_show(GTK_WIDGET(puss_app->search_dlg));
	gtk_widget_grab_focus(GTK_WIDGET(puss_app->search_dlg));
}

