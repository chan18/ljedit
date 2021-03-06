// DocSearch.c
// 

#include "DocSearch.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>

#include "DocManager.h"

#if GTK_MAJOR_VERSION==2
	#include <gtksourceview/gtksourceiter.h>
	#define gtk_text_iter_forward_search		gtk_source_iter_forward_search
	#define gtk_text_iter_backward_search		gtk_source_iter_backward_search
	#define GTK_TEXT_SEARCH_CASE_INSENSITIVE	GTK_SOURCE_SEARCH_CASE_INSENSITIVE
	#define GDK_KEY_Return		GDK_Return
	#define GDK_KEY_KP_Enter	GDK_KP_Enter
	#define GDK_KEY_Down		GDK_Down
	#define GDK_KEY_Up			GDK_Up
#endif

typedef struct {
	GtkDialog*	dlg;
	GtkEntry*	find_entry;
	GtkEntry*	replace_entry;
} FindDialog;

static const gchar* PUSS_LAST_SEARCH_TEXT = "puss:last_search_text";

static FindDialog g_self;

static gboolean find_next_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current, GtkTextSearchFlags flags) {
	GtkTextIter iter, end;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_mark(buf, "puss:searched_mark_start"));
	if( skip_current )
		gtk_text_iter_forward_char(&iter);
	gtk_text_buffer_get_end_iter(buf, &end);

	if( !gtk_text_iter_forward_search(&iter, text, flags, ps, pe, &end) ) {
		gtk_text_buffer_get_start_iter(buf, &iter);

		if( !gtk_text_iter_forward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static gboolean find_prev_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current, GtkTextSearchFlags flags) {
	GtkTextIter iter, end;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_mark(buf, "puss:searched_mark_start"));
	gtk_text_buffer_get_start_iter(buf, &end);

	if( !gtk_text_iter_backward_search(&iter, text, flags, ps, pe, &end) ) {
		gtk_text_buffer_get_end_iter(buf, &iter);

		if( !gtk_text_iter_backward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static void unmark_all_search_text_matched(GtkTextBuffer* buf, int search_flags) {
	GtkTextIter ps, pe;
	gtk_text_buffer_get_start_iter(buf, &ps);
	gtk_text_buffer_get_end_iter(buf, &pe);
	gtk_text_buffer_remove_tag_by_name(buf, "puss:searched", &ps, &pe);
}

static void mark_all_search_text_matched(GtkTextBuffer* buf, const gchar* text, int search_flags) {
	GtkTextIter ps, pe, end;
	gchar* last_search_text = g_object_get_data(G_OBJECT(buf), PUSS_LAST_SEARCH_TEXT);
	if( last_search_text ) {
		if( !text || !g_str_equal(text, last_search_text) )
			unmark_all_search_text_matched(buf, search_flags);
	}

	if( text && *text!='\0' ) {
		last_search_text = g_strdup(text);
		g_object_set_data_full(G_OBJECT(buf), PUSS_LAST_SEARCH_TEXT, last_search_text, g_free); 
		if( last_search_text ) {
			gtk_text_buffer_get_start_iter(buf, &ps);
			gtk_text_buffer_get_end_iter(buf, &end);

			while( gtk_text_iter_forward_search(&ps, last_search_text, search_flags, &ps, &pe, &end) ) {
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
	GtkTextMark* mark_s;
	GtkTextMark* mark_e;
	
	gboolean res = FALSE;
	gchar* last_search_text = 0;

	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
	pe = ps;

	mark_s = gtk_text_buffer_get_mark(buf, "puss:searched_mark_start");
	mark_e = gtk_text_buffer_get_mark(buf, "puss:searched_mark_end");
	last_search_text = g_object_get_data(G_OBJECT(buf), PUSS_LAST_SEARCH_TEXT);
	if( mark_current && last_search_text ) {
		gtk_text_buffer_get_iter_at_mark(buf, &ps, mark_s);
		gtk_text_buffer_get_iter_at_mark(buf, &pe, mark_e);
		gtk_text_buffer_remove_tag_by_name(buf, "puss:searched_current", &ps, &pe);
	}

	if( !is_continue ) {
		gtk_text_buffer_get_selection_bounds(buf, &ps, &pe);
		gtk_text_buffer_move_mark(buf, mark_s, &ps);
		gtk_text_buffer_move_mark(buf, mark_e, &pe);
	}

	if( text ) {
		GtkTextIter last_iter = ps;

		res = is_forward
			? find_next_text(view, text, &ps, &pe, skip_current, search_flags)
			: find_prev_text(view, text, &ps, &pe, skip_current, search_flags);

		if( res ) {
			//gtk_text_view_scroll_to_iter(view, &ps, 0.0, FALSE, 1.0, 0.25);
			if( mark_all )
				mark_all_search_text_matched(buf, text, search_flags);

			gtk_text_buffer_move_mark(buf, mark_s, &ps);
			gtk_text_buffer_move_mark(buf, mark_e, &pe);

			// move view point to left
			if( !gtk_text_iter_equal(&last_iter, &ps) ) {
				GtkAdjustment* vadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gtk_widget_get_parent(GTK_WIDGET(view))));
				gtk_adjustment_set_value(vadj, 0.0);
				gtk_text_view_scroll_mark_onscreen(view, mark_e);
				gtk_text_view_scroll_mark_onscreen(view, mark_s);
			}

			gtk_text_buffer_place_cursor(buf, &ps);
			if( mark_current )
				gtk_text_buffer_apply_tag_by_name(buf, "puss:searched_current", &ps, &pe);
		}
	}

	if( !res && mark_all )
		unmark_all_search_text_matched(buf, search_flags);

	return res;
}

gboolean puss_find_dialog_init(GtkBuilder* builder) {
	memset(&g_self, 0, sizeof(g_self));

	g_self.dlg = GTK_DIALOG(gtk_builder_get_object(builder, "search_dialog"));
	g_self.find_entry = GTK_ENTRY(gtk_builder_get_object(builder, "search_dialog_find_entry"));
	g_self.replace_entry = GTK_ENTRY(gtk_builder_get_object(builder, "search_dialog_replace_entry"));

	if( g_self.dlg && g_self.find_entry && g_self.replace_entry ) {
		gtk_widget_show_all( gtk_dialog_get_content_area(g_self.dlg) );
		return TRUE;
	}

	return FALSE;
}

void puss_find_dialog_show(const gchar* text) {
	gint page_num;
	GtkTextView* view;

	if( text )
		gtk_entry_set_text(g_self.find_entry, text);

	gtk_widget_show(GTK_WIDGET(g_self.dlg));
	gdk_window_raise(gtk_widget_get_window(GTK_WIDGET(g_self.dlg))); 

	gtk_widget_grab_focus( GTK_WIDGET(g_self.find_entry) );

	page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	view = puss_doc_get_view_from_page_num(page_num);

	puss_find_and_locate_text(view, text, TRUE, FALSE, TRUE, TRUE, FALSE, (GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE));
}

SIGNAL_CALLBACK gboolean puss_search_dialog_cb_find_entry_key_press( GtkWidget* widget, GdkEventKey* event ) {
	gint page_num;
	const gchar* text;
	GtkTextView* view;

	text = gtk_entry_get_text(g_self.find_entry);
	page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	switch( event->keyval ) {
	case GDK_KEY_Return:
	case GDK_KEY_KP_Enter:
	case GDK_KEY_Down:
		puss_find_and_locate_text(view, text, TRUE, TRUE, TRUE, TRUE, TRUE, (GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE));
		return TRUE;

	case GDK_KEY_Up:
		puss_find_and_locate_text(view, text, FALSE, TRUE, TRUE, TRUE, TRUE, (GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE));
		return TRUE;
	}

	return FALSE;
}

SIGNAL_CALLBACK void puss_search_dialog_cb_find(GtkButton* button) {
	gint page_num;
	const gchar* text;
	GtkTextView* view;

	text = gtk_entry_get_text(g_self.find_entry);
	page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return;

	puss_find_and_locate_text(view, text, TRUE, TRUE, TRUE, TRUE, TRUE, (GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE));
}

SIGNAL_CALLBACK void puss_search_dialog_cb_replace(GtkButton* button) {
	gint page_num;
	GtkTextView* view;
	GtkTextBuffer* buf;
	GtkTextIter ps, pe;
	gchar* last_search_text = 0;
	const gchar* find_text;
	const gint flags = (GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE);

	page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return;

	find_text = gtk_entry_get_text(g_self.find_entry);
	buf = gtk_text_view_get_buffer(view);
	last_search_text = g_object_get_data(G_OBJECT(buf), PUSS_LAST_SEARCH_TEXT);

	gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_mark(buf, "puss:searched_mark_start"));
	gtk_text_buffer_get_iter_at_mark(buf, &pe, gtk_text_buffer_get_mark(buf, "puss:searched_mark_end"));
	if( !gtk_text_iter_equal(&ps, &pe) && last_search_text ) {
		gchar* txt = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		gboolean equ = g_str_equal(find_text, txt);
		g_free(txt);

		if( equ ) {
			const gchar* replace_text = gtk_entry_get_text(g_self.replace_entry);

			gtk_text_buffer_begin_user_action(buf);
			gtk_text_buffer_delete(buf, &ps, &pe);
			gtk_text_buffer_insert(buf, &ps, replace_text, -1);
			gtk_text_buffer_end_user_action(buf);
		}
	}

	puss_find_and_locate_text(view, find_text, TRUE, TRUE, TRUE, TRUE, TRUE, flags);
}

SIGNAL_CALLBACK void puss_search_dialog_cb_replace_all(GtkButton* button) {
	gint page_num;
	GtkTextView* view;
	GtkTextBuffer* buf;
	GtkTextIter iter, ps, pe;
	const gchar* find_text;
	const gchar* replace_text;
	gint replace_text_len;
	const gint flags = (GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE);

	page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return;

	find_text = gtk_entry_get_text(g_self.find_entry);
	replace_text = gtk_entry_get_text(g_self.replace_entry);
	buf = gtk_text_view_get_buffer(view);

	gtk_text_buffer_get_start_iter(buf, &iter);

	gtk_text_buffer_begin_user_action(buf);
	{
		replace_text_len = (gint)strlen(replace_text);
		while( gtk_text_iter_forward_search(&iter
				, find_text
				, flags
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

