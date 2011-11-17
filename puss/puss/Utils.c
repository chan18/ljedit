// Utils.c
//

#include "Utils.h"
#include "Puss.h"
#include "OptionManager.h"

#include <string.h>
#include <memory.h>
#include <assert.h>


typedef struct _Utils          Utils;

struct _Utils {
	gchar** charset_list;
};

static Utils* puss_utils = 0;

static void parse_charset_list_option(const Option* option, const gchar* old, gpointer tag) {
	g_strfreev(puss_utils->charset_list);
	puss_utils->charset_list = g_strsplit_set(option->value, " \t,;", 0);
	//for( char** p=puss_utils->charset_list; *p; ++p )
	//	printf("%s\n", *p);
}

gboolean puss_utils_create() {
	const Option* option;

	puss_utils = g_new0(Utils, 1);

	option = puss_option_manager_option_reg("puss", "fileloader.charset_list", "GBK");
	puss_option_manager_monitor_reg(option, &parse_charset_list_option, 0, 0);
	parse_charset_list_option(option, 0, 0);

	return TRUE;
}

void puss_utils_destroy() {
	if( puss_utils ) {
		g_strfreev(puss_utils->charset_list);

		g_free(puss_utils);
		puss_utils = 0;
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

static gboolean load_convert_text(gchar** text, gsize* len, const gchar* charset, GError** err) {
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

static gboolean load_bom_convert_text(gchar** text, gsize* len, const gchar** charset, GError** err) {
	// UTF-8		EF BB BF
	// UTF-16(BE)	FE FF
	// UTF-16(LE)	FF FE
	// UTF-32(BE)	00 00 FE FF
	// UTF-32(LE)	FF FE 00 00
	// !!!NOT TEST // UTF-7		2B 2F 76, and one of the following: [ 38 | 39 | 2B | 2F ]
	// !!!NOT TEST // UTF-1		F7 64 4C
	// !!!NOT TEST // UTF-EBCDIC	DD 73 66 73
	// !!!NOT TEST // SCSU			0E FE FF
	// !!!NOT TEST // BOCU-1		FB EE 28 optionally followed by FF
	// GB-18030		84 31 95 33

	gchar* sbuf = *text;
	gsize slen = *len;
	gsize bytes_written = 0;

	if( slen>=3 && memcmp(sbuf, "\xEF\xBB\xBF", 3)==0 ) {
		*charset = "UTF-8";
		if( g_utf8_validate(sbuf+3, slen-3, 0) ) {
			sbuf = g_strdup(sbuf+3);
			slen -= 3;
			goto succeed;
		}

	} else if( slen>=4 && memcmp(sbuf, "\x84\x31\x95\x33", 4)==0 ) {
		*charset = "GB-18030";
		sbuf = g_convert(sbuf, slen, "UTF-8", *charset, 0, &bytes_written, err);
		if( sbuf ) {
			slen = bytes_written;
			goto succeed;
		}

	} else if( slen>=4 && memcmp(sbuf, "\x00\x00\xFE\xFF", 4)==0 ) {
		*charset = "UTF-32BE";
		sbuf = g_convert(sbuf, slen, "UTF-8", *charset, 0, &bytes_written, err);
		if( sbuf ) {
			slen = bytes_written;
			goto succeed;
		}

	} else if( slen>=4 && memcmp(sbuf, "\xFF\xFE\x00\x00", 4)==0 ) {
		*charset = "UTF-32LE";
		sbuf = g_convert(sbuf, slen, "UTF-8", *charset, 0, &bytes_written, err);
		if( sbuf ) {
			slen = bytes_written;
			goto succeed;
		}

	} else if( slen>=2 && memcmp(sbuf, "\xFE\xFF", 2)==0 ) {
		*charset = "UTF-16BE";
		sbuf = g_convert(sbuf, slen, "UTF-8", *charset, 0, &bytes_written, err);
		if( sbuf ) {
			slen = bytes_written;
			goto succeed;
		}

	} else if( slen>=2 && memcmp(sbuf, "\xFF\xFE", 2)==0 ) {
		*charset = "UTF-16LE";
		sbuf = g_convert(sbuf, slen, "UTF-8", *charset, 0, &bytes_written, err);
		if( sbuf ) {
			slen = bytes_written;
			goto succeed;
		}
	}

	return FALSE;

succeed:
	g_free(*text);
	*text = sbuf;
	*len = slen;
	return TRUE;
}

gboolean puss_save_file(const gchar* filename, const gchar* text, gssize len, const gchar* charset, gboolean use_BOM) {
	gboolean succeed = FALSE;
	GIOChannel* channel = 0;
	const gchar* BOM_str = 0;
	gsize BOM_len = 0;
	GError* err = 0;
	gsize bytes_written = 0;
	gchar* buf = g_convert(text, len, charset, "UTF-8", 0, &bytes_written, &err);
	if( !buf )
		goto finished;

	if( use_BOM ) {
		// UTF-8		EF BB BF
		// UTF-16(BE)	FE FF
		// UTF-16(LE)	FF FE
		// UTF-32(BE)	00 00 FE FF
		// UTF-32(LE)	FF FE 00 00
		// !!!NOT USE // UTF-7		2B 2F 76, and one of the following: [ 38 | 39 | 2B | 2F ]
		// !!!NOT USE // UTF-1		F7 64 4C
		// !!!NOT USE // UTF-EBCDIC	DD 73 66 73
		// !!!NOT USE // SCSU			0E FE FF
		// !!!NOT USE // BOCU-1		FB EE 28 optionally followed by FF
		// GB-18030		84 31 95 33

		if( g_str_equal(charset, "UTF-8") ) {
			BOM_str = "\xEF\xBB\xBF";
			BOM_len = 3;

		} else if( g_str_equal(charset, "GB-18030") ) {
			BOM_str = "\x84\x31\x95\x33";
			BOM_len = 4;

		} else if( g_str_equal(charset, "UTF-32BE") ) {
			BOM_str = "\x00\x00\xFE\xFF";
			BOM_len = 4;

		} else if( g_str_equal(charset, "UTF-32LE") ) {
			BOM_str = "\xFF\xFE\x00\x00";
			BOM_len = 4;

		} else if( g_str_equal(charset, "UTF-16BE") ) {
			BOM_str = "\xFE\xFF";
			BOM_len = 2;

		} else if( g_str_equal(charset, "UTF-16LE") ) {
			BOM_str = "\xFF\xFE";
			BOM_len = 2;
		}
	}

	// save file
	channel = g_io_channel_new_file(filename, "w", 0);
	if( !channel )
		goto finished;

	if( g_io_channel_set_encoding(channel, NULL, &err)==G_IO_STATUS_ERROR )
		goto finished;

	if( BOM_str && g_io_channel_write_chars(channel, BOM_str, BOM_len, 0, &err)==G_IO_STATUS_ERROR )
		goto finished;

	if( g_io_channel_write_chars(channel, buf, (gssize)bytes_written, 0, &err)==G_IO_STATUS_ERROR )
		goto finished;

	succeed = TRUE;

finished:
	if( err ) {
		g_warning("puss save file failed : %s", err->message);
		g_error_free(err);
	}

	if( channel )
		g_io_channel_close(channel);
	g_free(buf);
	return succeed;
}

gboolean puss_load_file(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, gboolean* use_BOM) {
	gboolean BOM = FALSE;
	const gchar* cs = 0;
	const gchar* locale = 0;
	gchar* sbuf = 0;
	gsize  slen = 0;

	g_return_val_if_fail(filename && text && len , FALSE);
	g_return_val_if_fail(*filename, FALSE);

	if( !g_file_get_contents(filename, &sbuf, &slen, 0) )
		return FALSE;

	// BOM convert to UTF-8 test
	if( load_bom_convert_text(&sbuf, &slen, &cs, 0) ) {
		BOM = TRUE;
		goto load_succeed;
	}

	// UTF-8 tests
	if( g_utf8_validate(sbuf, slen, 0) ) {
		cs = "UTF-8";
		goto load_succeed;
	}

	// charset tests
	if( puss_utils->charset_list ) {
		gchar** p = puss_utils->charset_list;
		for( ; *p; ++p ) {
			cs = *p;
			if( cs[0]=='\0' )
				continue;

			if( load_convert_text(&sbuf, &slen, cs, 0) )
				goto load_succeed;
		}
	}

	// local tests
	if( !g_get_charset(&locale) ) {		// get locale charset, and not UTF-8
		if( load_convert_text(&sbuf, &slen, locale, 0) ) {
			cs = locale;
			goto load_succeed;
		}
	}

	g_free(sbuf);
	return FALSE;

load_succeed:
	if( use_BOM )
		*use_BOM = BOM;
	if( charset )
		*charset = cs;
	*text = sbuf;
	*len = slen;
	return TRUE;
}

#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

gchar* puss_format_filename(const gchar* filename) {
	gchar* res = 0;

#ifdef G_OS_WIN32
	WIN32_FIND_DATAW wfdd;
	HANDLE hfd;
	wchar_t wbuf[32768];
	gchar**  paths;
	wchar_t* wfname;
	gsize len;
	gsize i;
	gsize j;
	
	wfname = (wchar_t*)g_utf8_to_utf16(filename, -1, 0, 0, 0);
	if( wfname ) {
		len = GetFullPathNameW(wfname, 32768, wbuf, 0);
		len = GetLongPathNameW(wbuf, wbuf, 32768);
		g_free(wfname);

		paths = g_new(gchar*, 256);
		paths[0] = g_strdup("_:");
		paths[0][0] = toupper(filename[0]);
		j = 1;

		for( i=3; i<len; ++i ) {
			if( wbuf[i]=='\\' ) {
				wbuf[i] = '\0';

				hfd = FindFirstFileW(wbuf, &wfdd);
				if( hfd != INVALID_HANDLE_VALUE ) {
					paths[j++] = g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
					FindClose(hfd);
				}
				wbuf[i] = '\\';
			}
		}

		hfd = FindFirstFileW(wbuf, &wfdd);
		if( hfd != INVALID_HANDLE_VALUE ) {
			paths[j++] = g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
			FindClose(hfd);
		}
		paths[j] = 0;

		res = g_build_filenamev(paths);
		g_strfreev(paths);
	}

	if( !res )
		res = g_strdup(filename);

	assert( res );

#else
	gboolean succeed = TRUE;
	gchar** p;
	gchar* outs[256];
	gchar** pt = outs;
	gchar** paths = g_strsplit(filename, "/", 0);
	for( p=paths; succeed && *p; ++p ) {
		if( g_str_equal(*p, ".") ) {
			// ignore ./

		} else if( g_str_equal(*p, "..") ) {
			if( pt==outs )
				succeed = FALSE;
			else
				--pt;

		} else {
			*pt = *p;
			++pt;
		}
	}

	if( succeed ) {
		*pt = NULL;
		res = g_strjoinv("/", outs);

	} else {
		res = g_strdup(filename);
	}

	g_strfreev(paths);

#endif

	return res;
}

