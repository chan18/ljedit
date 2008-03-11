// Utils.cpp
//

#include "Utils.h"

void puss_send_focus_change(GtkWidget *widget, gboolean in) {
	// Cut and paste from gtkwindow.c

	GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

	g_object_ref (widget);
   
	if (in)
		GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	else
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	fevent->focus_change.type = GDK_FOCUS_CHANGE;
	fevent->focus_change.window = GDK_WINDOW (g_object_ref(widget->window));
	fevent->focus_change.in = in;
  
	gtk_widget_event (widget, fevent);
  
	g_object_notify (G_OBJECT (widget), "has-focus");

	g_object_unref (widget);
	gdk_event_free (fevent);
}

void puss_active_panel_page(GtkNotebook* panel, gint page_num) {
	GtkWidget* w = gtk_notebook_get_nth_page(panel, page_num);
	if( w ) {
		gtk_notebook_set_current_page(panel, page_num);
		puss_send_focus_change(w, FALSE);
		puss_send_focus_change(w, TRUE);
	}
}

gboolean load_convert_text(gchar** text, gsize* len, const gchar* charset, GError** err) {
	gsize bytes_written = 0;
	gchar* result = g_convert(*text, *len, "UTF-8", charset, 0, &bytes_written, err);
	if( result ) {
		g_free(*text);
		*text = result;
		*len = bytes_written;
		return TRUE;
	}

	return FALSE;
}

const gchar* charset_order_list[] = { "GBK", 0 };

gboolean puss_load_file(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, GError** err) {
	gchar* sbuf = 0;
	gsize  slen = 0;
	const gchar** cs = 0;

	g_return_val_if_fail(filename && text && len && err, FALSE);
	g_return_val_if_fail(*filename && !(*text) && !(*err), FALSE);

	if( !g_file_get_contents(filename, &sbuf, &slen, err) )
		return FALSE;

	if( g_utf8_validate(sbuf, slen, 0) ) {
		if( charset )
			*charset = "UTF-8";
		*text = sbuf;
		*len = slen;
		return TRUE;
	}

	if( !g_get_charset(cs) && *cs ) {		// get locale charset, and not UTF-8
		if( load_convert_text(&sbuf, &slen, *cs, err) ) {
			if( charset )
				*charset = *cs;
			*text = sbuf;
			*len = slen;
			return TRUE;
		}
	}

	for( cs=charset_order_list; *cs!=0; ++cs ) {
		if( err )
			g_clear_error(err);

		if( load_convert_text(&sbuf, &slen, *cs, err) ) {
			if( charset )
				*charset = *cs;
			*text = sbuf;
			*len = slen;
			return TRUE;
		}
	}

	g_free(sbuf);
	return FALSE;
}

