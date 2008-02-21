// DocManager.cpp
// 

#include "DocManager.h"

#include <glib/gi18n.h>

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceiter.h>
#include <string.h>

#include "IPuss.h"
#include "Puss.h"
#include "Utils.h"


void __free_g_string( GString* gstr ) {
	g_string_free(gstr, TRUE);
}

const gchar* scroll_mark_name = "scroll-to-pos-mark";

gboolean doc_scroll_to_pos( GtkTextView* view ) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextMark* mark = gtk_text_buffer_get_mark(buf, scroll_mark_name);
	if( mark ) {
		gtk_text_view_scroll_to_mark(view, mark, 0.3, FALSE, 0.0, 0.0);
		gtk_text_buffer_delete_mark(buf, mark);
	}
    return FALSE;
}

void doc_locate_page_line( Puss* app, gssize page_num, gint line, gint offset ) {
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line_offset(buf, &iter, line, offset);
	gtk_text_buffer_place_cursor(buf, &iter);

	GtkTextMark* mark = gtk_text_buffer_get_mark(buf, scroll_mark_name);
	if( mark )
		gtk_text_buffer_move_mark(buf, mark, &iter);
	else
		gtk_text_buffer_create_mark(buf, "scroll-to-pos-mark", &iter, TRUE);

	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (GSourceFunc)&doc_scroll_to_pos, g_object_ref(G_OBJECT(view)), &g_object_unref);
}

gboolean doc_load_convert_text(gchar** text, gsize* len, const gchar* charset, GError** err) {
	gsize bytes_written = 0;
	gchar* result = g_convert(*text, *len, "UTF-8", charset, 0, &bytes_written, err);
	if( result ) {
		g_assert( !(*err) );

		g_free(*text);
		*text = result;
		*len = bytes_written;
		return TRUE;
	}

	return FALSE;
}

const gchar* charset_order_list[] = { "GBK", 0 };

gboolean doc_load_file(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, GError** err) {
	gchar* sbuf = 0;
	gsize  slen = 0;
	const gchar** p = 0;

	g_return_val_if_fail(filename && text && len && err, FALSE);
	g_return_val_if_fail(*filename && !(*text) && !(*err), FALSE);

	if( !g_file_get_contents(filename, &sbuf, &slen, err) )
		return FALSE;

	if( g_utf8_validate(sbuf, slen, 0) ) {
		*charset = "UTF-8";
		*text = sbuf;
		*len = slen;
		return TRUE;
	}

	if( !g_get_charset(charset) && *charset ) {		// get locale charset, and not UTF-8
		if( doc_load_convert_text(&sbuf, &slen, *charset, err) ) {
			*text = sbuf;
			*len = slen;
			return TRUE;
		}
	}

	for( p=charset_order_list; *p!=0; ++p ) {
		g_clear_error(err);

		*charset = *p;

		if( doc_load_convert_text(&sbuf, &slen, *charset, err) ) {
			*text = sbuf;
			*len = slen;
			return TRUE;
		}
	}

	g_free(sbuf);
	return FALSE;
}

void doc_reset_page_label(GtkTextBuffer* buf, GtkLabel* label) {
	gboolean modified = gtk_text_buffer_get_modified(buf);
	GString* url = puss_doc_get_url(buf);
	gchar* text = url
		? g_path_get_basename(url->str)
		: g_strdup(_("Untitled"));

/*
#ifdef G_OS_WIN32
	wchar_t* wfname = (wchar_t*)::g_utf8_to_utf16(filekey.c_str(), -1, 0, 0, 0);
	if( wfname != 0 ) {
		WIN32_FIND_DATAW wfdd;
		HANDLE hfd = FindFirstFileW(wfname, &wfdd);
		if( hfd != INVALID_HANDLE_VALUE ) {
			gchar* fname = ::g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
			if( fname != 0 ) {
				filename = fname;
				g_free(fname);
			}
			FindClose(hfd);
		}
		g_free(wfname);
	}
#endif
*/

	if( modified ) {
		gchar* title = g_strdup_printf("%s*", text);
		gtk_label_set_text(label, title);
		g_free(title);

	} else {
		gtk_label_set_text(label, text);
	}

	g_free(text);
}

gint doc_open_page( Puss* app, GtkSourceBuffer* buf, gboolean active_page ) {
	gint page_num;
	GtkSourceView* view;
	GtkLabel* label;
	GtkWidget* tab;
	GtkWidget* page;
	GtkTextIter iter;

	view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(buf));
	g_object_unref(G_OBJECT(buf));

	label = GTK_LABEL(gtk_label_new(0));
	doc_reset_page_label(GTK_TEXT_BUFFER(buf), label);

	g_signal_connect(GTK_TEXT_BUFFER(buf), "modified-changed", (GCallback)&doc_reset_page_label, label);

	tab = GTK_WIDGET(label);
	gtk_widget_show_all(tab);

	page = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(page), GTK_WIDGET(view));
	gtk_widget_show_all(page);
	g_object_set_data(G_OBJECT(page), "puss-doc-view", view);

	page_num = gtk_notebook_append_page(app->main_window->doc_panel, page, tab);

	if( active_page ) {
		gtk_notebook_set_current_page(app->main_window->doc_panel, page_num);
		gtk_widget_grab_focus(GTK_WIDGET(view));
	}

	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(buf), &iter, 0);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buf), &iter);

	return page_num;
}

gint doc_open_file( Puss* app, const gchar* url ) {
	gint page_num = puss_doc_find_page_from_url(app, url);
	if( page_num < 0 ) {
		gchar* text = 0;
		gsize len = 0;
		const gchar* charset = 0;
		GError* err = 0;

		if( doc_load_file(url, &text, &len, &charset, &err) ) {
			GtkSourceBuffer* buf = gtk_source_buffer_new(0);
			gtk_source_buffer_begin_not_undoable_action(buf);
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buf), text, len);
			gtk_source_buffer_end_not_undoable_action(buf);
			gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buf), FALSE);
			g_free(text);

			puss_doc_set_url(GTK_TEXT_BUFFER(buf), url);
			puss_doc_set_charset(GTK_TEXT_BUFFER(buf), charset);

			page_num = doc_open_page(app, buf, TRUE);

		} else {
			g_assert( err );
			g_printerr("ERROR : %s", err->message);
			g_error_free(err);
		}
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

GtkTextBuffer* doc_get_current_buffer( Puss* app ) {
	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	return page_num < 0 ? 0 : puss_doc_get_buffer_from_page_num(app, page_num);
}

gboolean doc_save_page( Puss* app, gint page_num, gboolean is_save_as ) {
	GtkTextBuffer* buf;
	GString* url;
	gboolean result;
	gchar* title;
	GtkLabel* label;

	if( page_num < 0 )
		return TRUE;

	buf = puss_doc_get_buffer_from_page_num(app, page_num);
	if( !buf )
		return TRUE;

	if( !gtk_text_buffer_get_modified(buf) )
		return TRUE;

	url = puss_doc_get_url(buf);

	if( !url )
		is_save_as = TRUE;

	if( is_save_as ) {
		GtkWidget* dlg;
		gint res;

		dlg = gtk_file_chooser_dialog_new( "Save File"
			, app->main_window->window
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
		}

		gtk_widget_destroy(dlg);

		if( res != GTK_RESPONSE_ACCEPT )
			return FALSE;
	}

	// save file
	result = doc_save_file(buf, 0);

	// update label
	title = g_path_get_basename(url->str);
	label = puss_doc_get_label_from_page_num(app, page_num);
	gtk_label_set_text(label, title);
	g_free(title);

	return result;
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

GtkLabel* puss_doc_get_label_from_page_num( Puss* app, int page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(app->main_window->doc_panel, page_num);
	return page ? GTK_LABEL(gtk_notebook_get_tab_label(app->main_window->doc_panel, page)) : 0;
}

GtkTextView* puss_doc_get_view_from_page_num( Puss* app, gint page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(app->main_window->doc_panel, page_num);
	return page ? puss_doc_get_view_from_page(page) : 0;
}

GtkTextBuffer* puss_doc_get_buffer_from_page_num( Puss* app, gint page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(app->main_window->doc_panel, page_num);
	return page ? puss_doc_get_buffer_from_page(page) : 0;
}

gint puss_doc_find_page_from_url( Puss* app, const gchar* url ) {
	gint i;
	gint num;
	GtkTextBuffer* buf;
	const GString* gstr;
	gsize url_len = (gsize)strlen(url);

	num = gtk_notebook_get_n_pages(app->main_window->doc_panel);
	for( i=0; i<num; ++i ) {
		buf = puss_doc_get_buffer_from_page_num(app, i);
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

void puss_doc_new( Puss* app ) {
	GtkSourceBuffer* buf = gtk_source_buffer_new(0);
	puss_doc_set_charset(GTK_TEXT_BUFFER(buf), "UTF-8");

	doc_open_page(app, buf, TRUE);
}

gboolean puss_doc_open( Puss* app, const gchar* url, gint line, gint line_offset ) {
	gint page_num = -1;

	if( url ) {
		page_num = doc_open_file(app, url);
		if( page_num < 0 )
			return FALSE;

		doc_locate_page_line(app, page_num, line, line_offset);

	} else {
		gint res;
		GtkWidget* dlg = gtk_file_chooser_dialog_new( "Open File"
			, app->main_window->window
			, GTK_FILE_CHOOSER_ACTION_OPEN
			, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
			, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT
			, NULL );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

		GtkTextBuffer* buffer = doc_get_current_buffer(app);
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
			page_num = doc_open_file(app, filename);
			g_free(filename);
		}

		gtk_widget_destroy(dlg);
	}

	return TRUE;
}

gboolean puss_doc_locate( Puss* app, const gchar* url, gint line, gint line_offset ) {
	gint page_num = puss_doc_find_page_from_url(app, url);
	if( page_num < 0 )
		return FALSE;

	if( line <= 0 )
		return FALSE;

	doc_locate_page_line(app, page_num, line, line_offset);
	return TRUE;
}

void puss_doc_save_current( Puss* app, gboolean save_as ) {
	doc_save_page(app, gtk_notebook_get_current_page(app->main_window->doc_panel), FALSE);
}

void puss_doc_save_current_as( Puss* app ) {
	doc_save_page(app, gtk_notebook_get_current_page(app->main_window->doc_panel), TRUE);
}

gboolean puss_doc_close_current( Puss* app ) {
	GtkTextBuffer* buf;
	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	if( page_num < 0 )
		return TRUE;

	buf = puss_doc_get_buffer_from_page_num(app, page_num);
	if( gtk_text_buffer_get_modified(buf) ) {
		gint res;
		GtkWidget* dlg;

		dlg = gtk_message_dialog_new( app->main_window->window
			, GTK_DIALOG_MODAL
			, GTK_MESSAGE_QUESTION
			, GTK_BUTTONS_YES_NO
			, _("file modified, save it?") );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);

		res = gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);

		switch( res ) {
		case GTK_RESPONSE_YES:
			if( !doc_save_page(app, page_num, FALSE) )
				return FALSE;
			break;
		case GTK_RESPONSE_NO:
			break;
		default:
			return FALSE;
		}
	}

	gtk_notebook_remove_page(app->main_window->doc_panel, page_num);
	return TRUE;
}

void puss_doc_save_all( Puss* app ) {
	gint num = gtk_notebook_get_n_pages(app->main_window->doc_panel);
	for( gint i=0; i<num; ++i )
		doc_save_page(app, i, FALSE);
}

gboolean puss_doc_close_all( Puss* app ) {
	gboolean need_prompt = TRUE;
	gboolean save_file_sign = FALSE;

	while( gtk_notebook_get_n_pages(app->main_window->doc_panel) ) {
		GtkTextBuffer* buf = puss_doc_get_buffer_from_page_num(app, 0);
		if( !buf )
			continue;

		if( gtk_text_buffer_get_modified(buf) ) {
			if( need_prompt ) {
				GString* url = puss_doc_get_url(buf);

				GtkWidget* dlg = gtk_message_dialog_new( app->main_window->window
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

				switch( gtk_dialog_run(GTK_DIALOG(dlg)) ) {
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
				if( !doc_save_page(app, 0, FALSE) )
					return FALSE;
			}
		}

		gtk_notebook_remove_page(app->main_window->doc_panel, 0);
	}

	return TRUE;
}

