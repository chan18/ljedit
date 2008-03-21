// Utils.cpp
//

#include "Utils.h"
#include "Puss.h"
#include "OptionManager.h"

struct Utils {
	gchar** charset_list;
};

void parse_charset_list_option(const Option* option) {
	Utils* self = puss_app->utils;

	g_strfreev(self->charset_list);
	self->charset_list = g_strsplit_set(option->current_value, " \t,;", 0);
	//for( char** p=self->charset_list; *p; ++p )
	//	printf("%s\n", *p);
}

void charset_list_option_changed(const Option* option, const gchar* old, gpointer tag) {
	parse_charset_list_option(option);
}

gboolean puss_utils_create() {
	puss_app->utils = g_new0(Utils, 1);
	if( !puss_app->utils ) {
		g_printerr("ERROR(puss) : utils create failed!\n");
		return FALSE;
	}

	const Option* option = puss_option_manager_option_reg("puss", "fileloader.charset_list", "GBK", 0, 0);
	parse_charset_list_option(option);
	puss_option_manager_monitor_reg(option, &charset_list_option_changed, 0);

	return TRUE;
}

void puss_utils_destroy() {
	Utils* self = puss_app->utils;
	if( self ) {
		g_strfreev(self->charset_list);

		g_free(self);
	}
}

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
		if( g_utf8_validate(result, bytes_written, 0) ) {
			g_free(*text);
			*text = result;
			*len = bytes_written;
			return TRUE;
		}

		g_free(result);
	}

	return FALSE;
}

gboolean puss_load_file(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset) {
	gchar* sbuf = 0;
	gsize  slen = 0;

	g_return_val_if_fail(filename && text && len , FALSE);
	g_return_val_if_fail(*filename, FALSE);

	if( !g_file_get_contents(filename, &sbuf, &slen, 0) )
		return FALSE;

	if( g_utf8_validate(sbuf, slen, 0) ) {
		if( charset )
			*charset = "UTF-8";
		*text = sbuf;
		*len = slen;
		return TRUE;
	}

	Utils* self = puss_app->utils;
	if( self->charset_list ) {
		for( gchar** cs=self->charset_list; *cs; ++cs ) {
			if( (*cs)[0]=='\0' )
				continue;

			if( load_convert_text(&sbuf, &slen, *cs, 0) ) {
				if( charset )
					*charset = *cs;
				*text = sbuf;
				*len = slen;
				return TRUE;
			}
		}
	}

	const gchar* locale = 0;
	if( !g_get_charset(&locale) ) {		// get locale charset, and not UTF-8
		if( load_convert_text(&sbuf, &slen, locale, 0) ) {
			if( charset )
				*charset = locale;
			*text = sbuf;
			*len = slen;
			return TRUE;
		}
	}

	g_free(sbuf);
	return FALSE;
}

