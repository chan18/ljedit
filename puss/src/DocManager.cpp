// DocManager.cpp
// 

#include "DocManager.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <string.h>

#include "Puss.h"
#include "Utils.h"
#include "GlobMatch.h"
#include "PosLocate.h"
#include "OptionManager.h"

void __free_g_string( GString* gstr ) {
	g_string_free(gstr, TRUE);
}

gboolean doc_close_page( gint page_num );

const gchar* scroll_mark_name = "scroll-to-pos-mark";

gboolean doc_scroll_to_pos( GtkTextView* view ) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextMark* mark = gtk_text_buffer_get_mark(buf, scroll_mark_name);
	if( mark ) {
		gtk_text_view_scroll_to_mark(view, mark, 0.0, TRUE, 1.0, 0.25);
		gtk_text_buffer_delete_mark(buf, mark);
	}
    return FALSE;
}

void doc_locate_page_line( gint page_num, gint line, gint offset ) {
	if( line <  0 )
		return;

	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	GtkTextIter iter;
	if( line >= gtk_text_buffer_get_line_count(buf) ) {
		gtk_text_buffer_get_end_iter(buf, &iter);

	} else {
		gtk_text_buffer_get_iter_at_line(buf, &iter, line);
		if( offset < 0 ) {
			GtkTextIter insert_iter;
			gtk_text_buffer_get_iter_at_mark(buf, &insert_iter, gtk_text_buffer_get_insert(buf));
			offset = gtk_text_iter_get_line_offset(&insert_iter);
		}

		if( offset < gtk_text_iter_get_bytes_in_line(&iter) )
			gtk_text_iter_set_line_offset(&iter, offset);
		else if( !gtk_text_iter_ends_line(&iter) )
			gtk_text_iter_forward_to_line_end(&iter);
	}

	gtk_text_buffer_place_cursor(buf, &iter);

	GtkTextMark* mark = gtk_text_buffer_get_mark(buf, scroll_mark_name);
	if( mark )
		gtk_text_buffer_move_mark(buf, mark, &iter);
	else
		gtk_text_buffer_create_mark(buf, "scroll-to-pos-mark", &iter, TRUE);

	puss_active_panel_page(puss_app->doc_panel, page_num);
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (GSourceFunc)&doc_scroll_to_pos, g_object_ref(G_OBJECT(view)), &g_object_unref);
}

void doc_reset_page_label(GtkTextBuffer* buf, GtkLabel* label) {
	gboolean modified = gtk_text_buffer_get_modified(buf);
	GString* url = puss_doc_get_url(buf);
	gchar* text = 0;

#ifdef G_OS_WIN32
	if( url ) {
		wchar_t* wfname = (wchar_t*)g_utf8_to_utf16(url->str, url->len, 0, 0, 0);
		if( wfname != 0 ) {
			WIN32_FIND_DATAW wfdd;
			HANDLE hfd = FindFirstFileW(wfname, &wfdd);
			if( hfd != INVALID_HANDLE_VALUE ) {
				text = g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
				FindClose(hfd);
			}
			g_free(wfname);
		}
	}
#endif

	if( !text ) {
		text = url
			? g_path_get_basename(url->str)
			: g_strdup(_("Untitled"));
	}

	if( modified ) {
		gchar* title = g_strdup_printf("%s*", text);
		gtk_label_set_text(label, title);
		g_free(title);

	} else {
		gtk_label_set_text(label, text);
	}

	g_free(text);
}

gboolean doc_cb_button_release_on_label(GtkWidget* widget, GdkEventButton *event) {
	if( event->button==2 ) {
		gint count = gtk_notebook_get_n_pages(puss_app->doc_panel);
		for( gint i=0; i < count; ++i ) {
			GtkWidget* page = gtk_notebook_get_nth_page(puss_app->doc_panel, i);
			GtkWidget* label = gtk_notebook_get_tab_label(puss_app->doc_panel, page);
			if( widget==label ) {
				doc_close_page(i);
				break;
			}
		}

		return TRUE;
	}

	return FALSE;
}

gint doc_open_page(GtkSourceBuffer* buf, gboolean active_page) {
	gint page_num;
	GtkSourceView* view;
	GtkLabel* label;
	GtkWidget* tab;
	GtkWidget* page;
	GtkTextIter iter;

	gtk_source_buffer_set_highlight_matching_brackets(buf, TRUE);

	view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(buf));
	g_object_unref(G_OBJECT(buf));
	gtk_source_view_set_auto_indent(view, TRUE);
	gtk_source_view_set_highlight_current_line(view, TRUE);
	gtk_source_view_set_show_line_numbers(view, TRUE);
	gtk_source_view_set_smart_home_end(view, GTK_SOURCE_SMART_HOME_END_BEFORE);
	gtk_source_view_set_right_margin_position(view, 120);
	gtk_source_view_set_show_right_margin(view, TRUE);
	gtk_source_view_set_tab_width(view, 4);

	const Option* font_option = puss_option_manager_find("puss", "editor.font");
	if( font_option && font_option->value) {
		PangoFontDescription* desc = pango_font_description_from_string(font_option->value);
		if( desc ) {
			gtk_widget_modify_font(GTK_WIDGET(view), desc);
			pango_font_description_free(desc);
		}
	}

	const Option* style_option = puss_option_manager_find("puss", "editor.style");
	if( style_option ) {
		GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
		GtkSourceStyleScheme* style = gtk_source_style_scheme_manager_get_scheme(ssm, style_option->value);
		if( style )
			gtk_source_buffer_set_style_scheme(buf, style);
	}

	label = GTK_LABEL(gtk_label_new(0));
	doc_reset_page_label(GTK_TEXT_BUFFER(buf), label);
	g_signal_connect(buf, "modified-changed", G_CALLBACK(&doc_reset_page_label), label);

	tab = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(tab), FALSE);
	gtk_container_add(GTK_CONTAINER(tab), GTK_WIDGET(label));
	g_object_set_data(G_OBJECT(tab), "puss-doc-label", label);

	gtk_widget_show_all(tab);

	page = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(page), GTK_WIDGET(view));
	gtk_widget_show_all(page);
	g_object_set_data(G_OBJECT(page), "puss-doc-view", view);

	page_num = gtk_notebook_append_page(puss_app->doc_panel, page, tab);
	gtk_notebook_set_tab_reorderable(puss_app->doc_panel, page, TRUE);

	g_signal_connect(tab, "button-release-event", G_CALLBACK(&doc_cb_button_release_on_label), 0);

	if( active_page ) {
		gtk_notebook_set_current_page(puss_app->doc_panel, page_num);
		gtk_widget_grab_focus(GTK_WIDGET(view));
	}

	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(buf), &iter, 0);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buf), &iter);

	return page_num;
}

gint doc_open_file( const gchar* filename, gint line, gint line_offset, gboolean show_message_if_open_failed ) {
	gchar* url = puss_format_filename(filename);
	gint page_num = puss_doc_find_page_from_url(url);
	if( page_num < 0 ) {
		gchar* text = 0;
		gsize len = 0;
		const gchar* charset = 0;

		if( puss_load_file(url, &text, &len, &charset) ) {
			// create text buffer & set text
			GtkSourceBuffer* buf = gtk_source_buffer_new(0);
			gtk_source_buffer_begin_not_undoable_action(buf);
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buf), text, len);
			gtk_source_buffer_end_not_undoable_action(buf);
			gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buf), FALSE);
			g_free(text);

			// save url & charset
			puss_doc_set_url(GTK_TEXT_BUFFER(buf), url);
			puss_doc_set_charset(GTK_TEXT_BUFFER(buf), charset);

			// select highlight language
			gtk_source_buffer_set_language(buf, puss_glob_get_language_by_filename(url));

			// create text view
			page_num = doc_open_page(buf, TRUE);

			if( line < 0 )
				line = 0;
		}

	} else {
		GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
		if( view ) {
			gtk_notebook_set_current_page(puss_app->doc_panel, page_num);
			gtk_widget_grab_focus(GTK_WIDGET(view));
		}
	}
	g_free(url);

	if( page_num < 0 ) {
		if( show_message_if_open_failed ) {
			GtkWidget* dlg = gtk_message_dialog_new( puss_app->main_window
				, GTK_DIALOG_MODAL
				, GTK_MESSAGE_ERROR
				, GTK_BUTTONS_OK
				, _("Error loading file : %s")
				, filename );
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), _("please select charset and retry!"));
			gtk_dialog_run(GTK_DIALOG(dlg));
			gtk_widget_destroy(dlg);
		}

	} else if( line >= 0 ) {
		doc_locate_page_line(page_num, line, line_offset);
		puss_pos_locate_add(page_num, line, line_offset);
	}

	return page_num;
}

gboolean doc_save_file( GtkTextBuffer* buf, GError** err ) {
	gunichar ch;
	GIOStatus status;
	GtkTextIter iter;

	const GString* url = puss_doc_get_url(buf);
	const GString* charset = puss_doc_get_charset(buf);
	GIOChannel* channel = g_io_channel_new_file(url->str, "w", err);
	if( !channel )
		return FALSE;

	status = g_io_channel_set_encoding(channel, charset->str, err);
	if( status==G_IO_STATUS_ERROR )
		return FALSE;

	gtk_text_buffer_get_iter_at_offset(buf, &iter, 0);
	if( !gtk_text_iter_is_end(&iter) ) {
		while( status!=G_IO_STATUS_ERROR ) {
			ch = gtk_text_iter_get_char(&iter);
			status = g_io_channel_write_unichar(channel, ch, err);

			if( !gtk_text_iter_forward_char(&iter) )
				break;
		}
	}

	g_io_channel_close(channel);
	if( status==G_IO_STATUS_ERROR )
		return FALSE;

	gtk_text_buffer_set_modified(buf, FALSE);
	return TRUE;
}

GtkTextBuffer* doc_get_current_buffer() {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	return page_num < 0 ? 0 : puss_doc_get_buffer_from_page_num(page_num);
}

gboolean doc_save_page( gint page_num, gboolean is_save_as ) {
	GtkTextBuffer* buf;
	GString* url;
	gboolean result;

	if( page_num < 0 )
		return TRUE;

	GtkWidget* page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);

	buf = puss_doc_get_buffer_from_page(page);
	if( !buf )
		return TRUE;

	if( !gtk_text_buffer_get_modified(buf) )
		if( !is_save_as )
			return TRUE;

	url = puss_doc_get_url(buf);

	if( !url )
		is_save_as = TRUE;

	if( is_save_as ) {
		GtkWidget* dlg;
		gint res;

		dlg = gtk_file_chooser_dialog_new( "Save File"
			, puss_app->main_window
			, GTK_FILE_CHOOSER_ACTION_SAVE
			, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
			, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT
			, NULL );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

		if( url ) {
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), url->str);

		} else {
			//gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), folder);
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), _("Untitled"));
		}

		res = gtk_dialog_run(GTK_DIALOG(dlg));
		if( res == GTK_RESPONSE_ACCEPT ) {
			gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
			if( url )
				g_string_assign(url, filename);
			else
				puss_doc_set_url(buf, filename);
			g_free (filename);

			GtkWidget* tab = gtk_notebook_get_tab_label(puss_app->doc_panel, page);
			GtkLabel* label = (GtkLabel*)g_object_get_data(G_OBJECT(tab), "puss-doc-label");
			doc_reset_page_label(buf, label);
		}

		gtk_widget_destroy(dlg);

		if( res != GTK_RESPONSE_ACCEPT )
			return FALSE;
	}

	// save file
	result = doc_save_file(buf, 0);

	return result;
}

gboolean doc_close_page( gint page_num ) {
	GtkTextBuffer* buf = puss_doc_get_buffer_from_page_num(page_num);
	if( buf && gtk_text_buffer_get_modified(buf) ) {
		gint res;
		GtkWidget* dlg;

		dlg = gtk_message_dialog_new( puss_app->main_window
			, GTK_DIALOG_MODAL
			, GTK_MESSAGE_QUESTION
			, GTK_BUTTONS_YES_NO
			, _("file modified, save it?") );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);

		res = gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);

		switch( res ) {
		case GTK_RESPONSE_YES:
			if( !doc_save_page(page_num, FALSE) )
				return FALSE;
			break;
		case GTK_RESPONSE_NO:
			break;
		default:
			return FALSE;
		}
	}

	gtk_notebook_remove_page(puss_app->doc_panel, page_num);
	puss_pos_locate_current();
	return TRUE;
}

void puss_doc_set_url( GtkTextBuffer* buffer, const gchar* url ) {
	g_object_set_data_full(G_OBJECT(buffer), "puss-doc-url", g_string_new(url), (GDestroyNotify)&__free_g_string);
}

GString* puss_doc_get_url( GtkTextBuffer* buffer ) {
	return (GString*)g_object_get_data(G_OBJECT(buffer), "puss-doc-url");
}

void puss_doc_set_charset( GtkTextBuffer* buffer, const gchar* charset ) {
	g_object_set_data_full(G_OBJECT(buffer), "puss-doc-charset", g_string_new(charset), (GDestroyNotify)&__free_g_string);
}

GString* puss_doc_get_charset( GtkTextBuffer* buffer ) {
	return (GString*)g_object_get_data(G_OBJECT(buffer), "puss-doc-charset");
}

void puss_doc_replace_all( GtkTextBuffer* buf
		, const gchar* find_text
		, const gchar* replace_text
		, gint flags )
{
	GtkTextIter iter;
	gtk_text_buffer_get_start_iter(buf, &iter);

	gtk_text_buffer_begin_user_action(buf);
	{
		gint replace_text_len = (gint)strlen(replace_text);

		GtkTextIter ps, pe;
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

GtkTextView* puss_doc_get_view_from_page( GtkWidget* page ) {
	gpointer tag = g_object_get_data(G_OBJECT(page), "puss-doc-view");
	return (tag && GTK_TEXT_VIEW(tag)) ? GTK_TEXT_VIEW(tag) : 0;
}

GtkTextBuffer* puss_doc_get_buffer_from_page( GtkWidget* page ) {
	GtkTextView* view = puss_doc_get_view_from_page(page);
	return view ? gtk_text_view_get_buffer(view) : 0;
}

GtkTextView* puss_doc_get_view_from_page_num( gint page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);
	return page ? puss_doc_get_view_from_page(page) : 0;
}

GtkTextBuffer* puss_doc_get_buffer_from_page_num( gint page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);
	return page ? puss_doc_get_buffer_from_page(page) : 0;
}

gint puss_doc_find_page_from_url( const gchar* url ) {
	gint i;
	gint num;
	GtkTextBuffer* buf;
	const GString* gstr;
	gsize url_len = (gsize)strlen(url);

	num = gtk_notebook_get_n_pages(puss_app->doc_panel);
	for( i=0; i<num; ++i ) {
		buf = puss_doc_get_buffer_from_page_num(i);
		if( !buf )
			continue;

		gstr = puss_doc_get_url(buf);
		if( !gstr )
			continue;

		if( gstr->len != url_len )
			continue;

		if( strcmp(gstr->str, url)==0 )
			return i;
	}

	return -1;
}

void puss_doc_new() {
	GtkSourceBuffer* buf = gtk_source_buffer_new(0);
	puss_doc_set_charset(GTK_TEXT_BUFFER(buf), "UTF-8");

	puss_pos_locate_add_current_pos();
	gint page_num = doc_open_page(buf, TRUE);
	puss_pos_locate_add(page_num, 0, 0);
}

gboolean puss_doc_open( const gchar* url, gint line, gint line_offset, gboolean show_message_if_open_failed ) {
	gint page_num = -1;
	puss_pos_locate_add_current_pos();

	if( url ) {
		page_num = doc_open_file(url, line, line_offset, show_message_if_open_failed);
		if( page_num < 0 )
			return FALSE;

	} else {
		gint res;
		GtkWidget* dlg = gtk_file_chooser_dialog_new( "Open File"
			, puss_app->main_window
			, GTK_FILE_CHOOSER_ACTION_OPEN
			, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
			, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT
			, NULL );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

		GtkTextBuffer* buffer = doc_get_current_buffer();
		if( buffer ) {
			const GString* gstr = puss_doc_get_url(buffer);
			if( gstr ) {
				gchar* folder = g_path_get_dirname(gstr->str);
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), folder);
				g_free(folder);
			}
		}

		res = gtk_dialog_run(GTK_DIALOG(dlg));
		if( res == GTK_RESPONSE_ACCEPT ) {
			gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
			page_num = doc_open_file(filename, line, line_offset, show_message_if_open_failed);
			line = -1;
			g_free(filename);
		}

		gtk_widget_destroy(dlg);
	}

	return TRUE;
}

gboolean puss_doc_locate( gint page_num, gint line, gint line_offset, gboolean add_pos_locate ) {
	if( page_num < 0 )
		return FALSE;

	if( line < 0 )
		return FALSE;

	if( add_pos_locate ) {
		puss_pos_locate_add_current_pos();
		doc_locate_page_line(page_num, line, line_offset);
		puss_pos_locate_add(page_num, line, line_offset);

	} else {
		doc_locate_page_line(page_num, line, line_offset);
	}

	return TRUE;
}

void puss_doc_save_current( gboolean save_as ) {
	doc_save_page(gtk_notebook_get_current_page(puss_app->doc_panel), save_as);
}

gboolean puss_doc_close_current() {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	return doc_close_page(page_num);
}

void puss_doc_save_all() {
	gint num = gtk_notebook_get_n_pages(puss_app->doc_panel);
	for( gint i=0; i<num; ++i )
		doc_save_page(i, FALSE);
}

gboolean puss_doc_close_all() {
	gboolean need_prompt = TRUE;
	gboolean save_file_sign = FALSE;

	GtkWindow* main_window = puss_app->main_window;
	GtkNotebook* doc_panel = puss_app->doc_panel;
	while( gtk_notebook_get_n_pages(doc_panel) ) {
		GtkTextBuffer* buf = puss_doc_get_buffer_from_page_num(0);
		if( buf && gtk_text_buffer_get_modified(buf) ) {
			if( need_prompt ) {
				GString* url = puss_doc_get_url(buf);

				GtkWidget* dlg = gtk_message_dialog_new( main_window
					, GTK_DIALOG_MODAL
					, GTK_MESSAGE_QUESTION
					, GTK_BUTTONS_NONE
					, "file modified, save it?\n\n%s\n"
					, url ? url->str : _("Untitled") );

				gtk_dialog_add_buttons( GTK_DIALOG(dlg)
					, _("yes to all"), GTK_RESPONSE_ACCEPT
					, _("no to all"), GTK_RESPONSE_CANCEL
					, GTK_STOCK_YES, GTK_RESPONSE_YES
					, GTK_STOCK_NO, GTK_RESPONSE_NO
					, NULL );

				gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);

				gint res = gtk_dialog_run(GTK_DIALOG(dlg));
				gtk_widget_destroy(dlg);

				switch( res ) {
				case GTK_RESPONSE_APPLY:
					need_prompt = FALSE;
					save_file_sign = TRUE;
					break;
				case GTK_RESPONSE_CANCEL:
					need_prompt = FALSE;
					save_file_sign = FALSE;
					break;
				case GTK_RESPONSE_YES:
					save_file_sign = TRUE;
					break;
				case GTK_RESPONSE_NO:
					save_file_sign = FALSE;
					break;
				default:
					return FALSE;
				}
			}

			if( save_file_sign ) {
				if( !doc_save_page(0, FALSE) )
					return FALSE;
			}
		}

		gtk_notebook_remove_page(doc_panel, 0);
	}

	return TRUE;
}

gboolean puss_doc_manager_create() {
	return TRUE;
}

void puss_doc_manager_destroy() {
}

