// DocManager.cpp
// 

#include "DocManager.h"
#include "Puss.h"

#include <glib/gi18n.h>

#include <gtksourceview/gtksourceview.h>
#include <string.h>

void __free_g_string( GString* gstr ) {
	g_string_free(gstr, TRUE);
}

typedef struct {
	Puss*	app;
	gint	page;
	gint	line;
	gint	offset;
} DocPos;

gboolean doc_scroll_to_pos( DocPos* pos ) {
	GtkTextView* view = GTK_TEXT_VIEW(puss_doc_get_view_from_page_num(pos->app, pos->page));
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line_offset(buf, &iter, pos->line, pos->offset);
	gtk_text_view_scroll_to_iter(view, &iter, 0.3, FALSE, 0.0, 0.0);

    return FALSE;
}

void doc_locate_page_line( Puss* app, gssize page_num, gint line, gint offset ) {
	gpointer pos = g_malloc(sizeof(DocPos));
	if( pos ) {
		DocPos* dp = (DocPos*)pos;
		dp->app = app;
		dp->page = page_num;
		dp->line = line;
		dp->offset = offset;
		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (GSourceFunc)&doc_scroll_to_pos, pos, &g_free);
	}
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

gint doc_open_page( Puss* app, GtkSourceBuffer* buf ) {
	GtkSourceView* view;
	GtkLabel* label;
	GtkWidget* tab;
	GtkWidget* page;

	if( buf ) {
		view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(buf));
		g_object_unref(G_OBJECT(buf));

	} else {
		view = GTK_SOURCE_VIEW(gtk_source_view_new());
	}

	label = GTK_LABEL(gtk_label_new(_("Untitled document")));

	tab = GTK_WIDGET(label);
	gtk_widget_show_all(tab);

	page = GTK_WIDGET(view);
	gtk_widget_show_all(page);
	g_object_set_data(G_OBJECT(page), "puss-doc-view", view);

	return gtk_notebook_append_page(app->ui.doc_panel, page, tab);
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
	while( status!=G_IO_STATUS_ERROR && !gtk_text_iter_is_end(&iter) ) {
		ch = gtk_text_iter_get_char(&iter);
		status = g_io_channel_write_unichar(channel, ch, err);
	}

	g_io_channel_close(channel);
	if( status==G_IO_STATUS_ERROR )
		return FALSE;

	gtk_text_buffer_set_modified(buf, FALSE);
	return TRUE;
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
			, app->ui.main_window
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
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), _("Untitled document"));
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

GtkTextView* puss_doc_get_view_from_page( GtkWidget* page ) {
	gpointer tag = g_object_get_data(G_OBJECT(page), "puss-doc-view");
	return (tag && GTK_TEXT_VIEW(tag)) ? GTK_TEXT_VIEW(tag) : 0;
}

GtkTextBuffer* puss_doc_get_buffer_from_page( GtkWidget* page ) {
	GtkTextView* view = puss_doc_get_view_from_page(page);
	return view ? gtk_text_view_get_buffer(view) : 0;
}

GtkLabel* puss_doc_get_label_from_page_num( Puss* app, int page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(app->ui.doc_panel, page_num);
	if( page ) {
		GtkLabel* label = GTK_LABEL(gtk_notebook_get_tab_label(app->ui.doc_panel, page));
		return label;
	}
	
	return 0;
}

GtkTextView* puss_doc_get_view_from_page_num( Puss* app, gint page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(app->ui.doc_panel, page_num);
	return page ? puss_doc_get_view_from_page(page) : 0;
}

GtkTextBuffer* puss_doc_get_buffer_from_page_num( Puss* app, gint page_num ) {
	GtkWidget* page = gtk_notebook_get_nth_page(app->ui.doc_panel, page_num);
	return page ? puss_doc_get_buffer_from_page(page) : 0;
}

gint puss_doc_find_page_from_url( Puss* app, const gchar* url ) {
	gint i;
	gint num;
	GtkTextBuffer* buf;
	const GString* gstr;
	gsize url_len = (gsize)strlen(url);

	num = gtk_notebook_get_n_pages(app->ui.doc_panel);
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
	doc_open_page(app, 0);
}

gboolean puss_doc_open( Puss* app, const gchar* url, gint line, gint line_offset ) {
	gint page_num = puss_doc_find_page_from_url(app, url);
	if( page_num < 0 ) {
		gchar* text = 0;
		gsize len = 0;
		const gchar* charset = 0;
		GError* err = 0;
		GtkSourceBuffer* buf;

		if( !doc_load_file(url, &text, &len, &charset, &err) ) {
			g_assert( err );
			g_printerr("ERROR : %s", err->message);
			g_error_free(err);

			return FALSE;
		}

		buf = gtk_source_buffer_new(0);
		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buf), text, len);
		g_free(text);

		puss_doc_set_url(GTK_TEXT_BUFFER(buf), url);
		puss_doc_set_charset(GTK_TEXT_BUFFER(buf), charset);

		page_num = doc_open_page(app, buf);
	}

	if( page_num < 0 )
		return FALSE;

	doc_locate_page_line(app, page_num, line, line_offset);
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
	doc_save_page(app, gtk_notebook_get_current_page(app->ui.doc_panel), FALSE);
}

void puss_doc_save_current_as( Puss* app ) {
	doc_save_page(app, gtk_notebook_get_current_page(app->ui.doc_panel), TRUE);
}

gboolean puss_doc_close_current( Puss* app ) {
	GtkTextBuffer* buf;
	gint page_num = gtk_notebook_get_current_page(app->ui.doc_panel);
	if( page_num < 0 )
		return TRUE;

	buf = puss_doc_get_buffer_from_page_num(app, page_num);
	if( gtk_text_buffer_get_modified(buf) ) {
		gint res;
		GtkWidget* dlg;

		dlg = gtk_message_dialog_new( app->ui.main_window
			, GTK_DIALOG_MODAL
			, GTK_MESSAGE_QUESTION
			, GTK_BUTTONS_YES_NO
			, _("file modified, save it?") );

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

	gtk_notebook_remove_page(app->ui.doc_panel, page_num);
	return TRUE;
}


void puss_doc_save_all( Puss* app ) {
}


void puss_doc_close_all( Puss* app ) {
}

