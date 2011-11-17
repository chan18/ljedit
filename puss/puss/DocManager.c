// DocManager.c
// 

// TODO : use gio!!!!
// 

#include "DocManager.h"

#ifdef G_OS_WIN32
#include <windows.h>
#include <gdk/gdkwin32.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#include <gio/gio.h>

#include <string.h>

#include "Puss.h"
#include "DocSearch.h"
#include "Utils.h"
#include "GlobMatch.h"
#include "PosLocate.h"
#include "OptionManager.h"

#define PUSS_TYPE_TEXT_VIEW             (puss_text_view_get_type ())
#define PUSS_TEXT_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PUSS_TYPE_TEXT_VIEW, GtkPussTextView))
#define PUSS_TEXT_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PUSS_TYPE_TEXT_VIEW, GtkPussTextViewClass))
#define PUSS_IS_TEXT_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PUSS_TYPE_TEXT_VIEW))
#define PUSS_IS_TEXT_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PUSS_TYPE_TEXT_VIEW))
#define PUSS_TEXT_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PUSS_TYPE_TEXT_VIEW, GtkPussTextViewClass))


typedef struct _GtkPussTextView GtkPussTextView;
typedef struct _GtkPussTextViewClass GtkPussTextViewClass;

struct _GtkPussTextView {
	GtkSourceView	parent;
	
#ifdef G_OS_WIN32
	WNDPROC			old_proc;
#endif
};

struct _GtkPussTextViewClass {
	GtkSourceViewClass parent_class;

	void (*search)(GtkPussTextView *view);
};

G_DEFINE_TYPE(GtkPussTextView, puss_text_view, GTK_TYPE_SOURCE_VIEW)

/* Signals */
enum {
	PUSS_SEARCH,
	LAST_SIGNAL
};

const gchar* mark_name_view_scroll = "puss:scroll_mark";


typedef  void (*MoveCursorFn)(GtkTextView *text_view, GtkMovementStep step, gint count, gboolean extend_selection);
typedef  gint (*ButtonPressFn)(GtkWidget *widget, GdkEventButton *event);

static guint signals[LAST_SIGNAL] = { 0 };
static MoveCursorFn parent_move_cursor_fn = 0;
static ButtonPressFn parent_press_event_fn = 0;

static void puss_text_view_search(GtkPussTextView *view);

#ifdef G_OS_WIN32
	static gboolean puss_text_view_cb_init(GtkWidget* widget, GdkEvent* event);
#endif

static void puss_text_view_move_cursor(GtkTextView *view, GtkMovementStep step, gint count, gboolean extend_selection);
static gint puss_text_view_button_press_event(GtkWidget *widget, GdkEventButton *event);

static void puss_text_view_class_init(GtkPussTextViewClass* klass) {
	GtkBindingSet* binding_set;
	GtkTextViewClass* parent_class = GTK_TEXT_VIEW_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_move_cursor_fn = parent_class->move_cursor;
	parent_press_event_fn = widget_class->button_press_event;
	parent_class->move_cursor = puss_text_view_move_cursor;
	widget_class->button_press_event = puss_text_view_button_press_event;

	binding_set = gtk_binding_set_by_class(klass);
	klass->search = puss_text_view_search;

	signals[PUSS_SEARCH] = g_signal_new( "puss_search"
		, G_TYPE_FROM_CLASS(klass)
		, G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION
		, G_STRUCT_OFFSET(GtkPussTextViewClass, search)
		, NULL
		, NULL
		, gtk_marshal_VOID__VOID
		, G_TYPE_NONE
		, 0 );

	gtk_binding_entry_add_signal(binding_set, GDK_F, GDK_CONTROL_MASK, "puss_search", 0);
}

static void doc_cb_move_focus(GtkWidget* widget, GtkDirectionType dir) {
	//GtkSourceView* view = GTK_SOURCE_VIEW(widget);
	g_signal_stop_emission_by_name(widget, "move-focus");

	switch( dir ) {
	case GTK_DIR_TAB_FORWARD:
		gtk_notebook_next_page(puss_app->doc_panel);
		break;
	case GTK_DIR_TAB_BACKWARD:
		gtk_notebook_prev_page(puss_app->doc_panel);
		break;
	default:
		break;
	}
}

static void puss_text_view_init(GtkPussTextView* view) {
	GtkSourceView* sview = GTK_SOURCE_VIEW(view);
	gtk_source_view_set_auto_indent(sview, TRUE);
	gtk_source_view_set_highlight_current_line(sview, TRUE);
	gtk_source_view_set_show_line_numbers(sview, TRUE);
	gtk_source_view_set_smart_home_end(sview, GTK_SOURCE_SMART_HOME_END_BEFORE);
	gtk_source_view_set_right_margin_position(sview, 120);
	gtk_source_view_set_show_right_margin(sview, TRUE);
	gtk_source_view_set_tab_width(sview, 4);

	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 3);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_NONE);

	g_signal_connect(view, "move-focus", G_CALLBACK(&doc_cb_move_focus), 0);

	// thinkpad trickpoint support
	// need : modify TP4table.dat, add and restart two SynTPxxx.exe files: 
	// ; Gtk+
	// *,*,*,gdkWindowTopLevel,*,gdkWindowChild,WheelStd,0,9
	// 
	// or add
	// ; puss
	// *,*,puss.exe,*,*,*,WheelVkey,0,9
	// 
	// but on win32 platform, use Spy++ only recv(WM_MOUSEWHEEL & WM_HSCROLL)
	// but Win GDK looks like not support those messages, now DIY this
	// 
#ifdef G_OS_WIN32
	{
		g_signal_connect(view, "realize", G_CALLBACK(&puss_text_view_cb_init), 0);
	}
#endif
}

/* Constructors */
static GtkWidget* puss_text_view_new_with_buffer(GtkSourceBuffer* buf) {
	GtkPussTextView* view = g_object_new(PUSS_TYPE_TEXT_VIEW, 0);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(view), GTK_TEXT_BUFFER(buf));
	return GTK_WIDGET(view);
}

#ifdef G_OS_WIN32

	LRESULT CALLBACK __trackpoint_win32_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		GtkPussTextView* self = (GtkPussTextView*)GetWindowLong(hwnd, GWL_USERDATA);
		if( msg==WM_MOUSEWHEEL ) {
			short delta = GET_WHEEL_DELTA_WPARAM(wparam);
			int line = delta / WHEEL_DELTA;
			GtkWidget* sc = gtk_widget_get_parent(GTK_WIDGET(self));
			g_print("wheel : %d %d %d %d\n", HIWORD(wparam), LOWORD(wparam), HIWORD(lparam), LOWORD(lparam));
			if( GTK_IS_SCROLLED_WINDOW(sc) ) {
				GtkScrolledWindow* scwin = GTK_SCROLLED_WINDOW(sc);
				GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment(scwin);
				if( adj )
					gtk_adjustment_set_value(adj, adj->value - (line * adj->step_increment / 3));
			}

		} else if( msg==WM_HSCROLL ) {
			// TODO : can not receive this message, but Spy++ got it. why??
			//g_print("got it(%d-%d-%d)!\n", msg, wparam, lparam);
		}

		return CallWindowProc(self->old_proc, hwnd, msg, wparam, lparam);
	}

	static gboolean puss_text_view_cb_init(GtkWidget* widget, GdkEvent* event) {
		// GtkPussTextView* self = PUSS_TEXT_VIEW(widget);
		// GdkWindow* win = gtk_widget_get_window(widget);
		// HWND hwnd = GDK_WINDOW_HWND(win);
		// NOTICE : not work on gtk+-2.20, now not use this
		// g_print("ss : %d\n", hwnd);
		// SetWindowLong(hwnd, GWL_USERDATA, (LONG)self);
		// self->old_proc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
		// self->old_proc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)__trackpoint_win32_proc);
		return TRUE;
	}
#endif

static void puss_text_view_search(GtkPussTextView *view) {
	gchar* text = 0;
	GtkTextBuffer* buf;
	GtkTextIter ps, pe;
	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) )
		text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);

	puss_find_dialog_show(text);

	g_free(text);
}

static void puss_text_view_move_cursor(GtkTextView *view, GtkMovementStep step, gint count, gboolean extend_selection) {
	(*parent_move_cursor_fn)(view, step, count, extend_selection);

	if( step==GTK_MOVEMENT_WORDS ) {
		// set select word include "_"
		// 
		// TODO : maybe use PangoLanguage extends will easy to fix this, but i don't understand it.
		// 

		gunichar ch;
		GtkTextBuffer* buf;
		GtkTextIter ps, pe;

		buf = gtk_text_view_get_buffer(view);
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
		gtk_text_buffer_get_iter_at_mark(buf, &pe, gtk_text_buffer_get_selection_bound(buf));

		if( count < 0 ) {
			if( !gtk_text_iter_ends_line(&ps) ) {
				ch = gtk_text_iter_get_char(&ps);
				if( ch=='_' || g_unichar_isalnum(ch) ) {
					while( ch=='_' || g_unichar_isalnum(ch) ) {
						ch = gtk_text_iter_backward_char(&ps)
							? gtk_text_iter_get_char(&ps)
							: 0;
					}
					if( ch )
						gtk_text_iter_forward_char(&ps);
				} else {
					while( !(ch=='_' || g_unichar_isalnum(ch)) ) {
						ch = gtk_text_iter_backward_char(&ps)
							? gtk_text_iter_get_char(&ps)
							: '_';
					}
					if( ch=='_' )
						gtk_text_iter_forward_char(&ps);
				}
			}

		} else if( count > 0 ) {
			if( !gtk_text_iter_ends_line(&ps) ) {
				ch = gtk_text_iter_get_char(&ps);
				if( ch=='_' || g_unichar_isalnum(ch) ) {
					while( ch=='_' || g_unichar_isalnum(ch) ) {
						if( !gtk_text_iter_forward_char(&ps) )
							break;
						ch = gtk_text_iter_get_char(&ps);
					}
				} else {
					while( !(ch=='_' || g_unichar_isalnum(ch)) ) {
						if( !gtk_text_iter_forward_char(&ps) )
							break;
						ch = gtk_text_iter_get_char(&ps);
					}
				}
			}
		}

		gtk_text_buffer_select_range(buf, &ps, extend_selection ? &pe : &ps);
	}
}

static gint puss_text_view_button_press_event(GtkWidget *widget, GdkEventButton *event) {
	GtkTextView* text_view = GTK_TEXT_VIEW(widget);
	gint res;
	if( text_view->need_im_reset ) {
		GTK_TEXT_VIEW(widget)->need_im_reset = FALSE;
		res = (*parent_press_event_fn)(widget, event);
		gtk_im_context_reset(text_view->im_context);
	} else {
		res = (*parent_press_event_fn)(widget, event);
	}


	if( event->button==1 && event->type == GDK_2BUTTON_PRESS ) {
		// Double-Click word can select word(include "_" )
		// but sorry that : after double-click, then drag whill change selection range
		// 

		// !!!	no public way to set GtkTextView drag selection, 
		//		so after double click selection range will change
		// 

		// TODO : maybe use PangoLanguage extends will easy to fix this, but i don't understand it.
		// 

		gunichar ch;
		GtkTextBuffer* buf;
		GtkTextIter ps, pe;

		buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
		gtk_text_buffer_get_iter_at_mark(buf, &pe, gtk_text_buffer_get_selection_bound(buf));

		if( !gtk_text_iter_ends_line(&ps) ) {
			ch = gtk_text_iter_get_char(&ps);
			if( ch=='_' || g_unichar_isalnum(ch) ) {
				while( ch=='_' || g_unichar_isalnum(ch) ) {
					ch = gtk_text_iter_backward_char(&ps)
						? gtk_text_iter_get_char(&ps)
						: 0;
				}
				if( ch )
					gtk_text_iter_forward_char(&ps);
			}
		}

		if( !gtk_text_iter_ends_line(&pe) ) {
			ch = gtk_text_iter_get_char(&pe);
			if( ch=='_' || g_unichar_isalnum(ch) ) {
				while( ch=='_' || g_unichar_isalnum(ch) ) {
					if( !gtk_text_iter_forward_char(&pe) )
						break;
					ch = gtk_text_iter_get_char(&pe);
				}
			}
		}

		gtk_text_buffer_select_range(buf, &pe, &ps);
	}

	return res;
}

void __free_g_string( GString* gstr ) {
	g_string_free(gstr, TRUE);
} 

typedef struct {
	GTimeVal mtime;
	goffset  size;
	gboolean need_check;
} ModifyInfo;

gboolean doc_file_get_mtime(gchar* url, ModifyInfo* mi) {
	gboolean result = FALSE;
	GFile* file = g_file_new_for_path(url);
	if( file ) {
		GFileInfo* info = g_file_query_info(file
			, G_FILE_ATTRIBUTE_STANDARD_TYPE","G_FILE_ATTRIBUTE_STANDARD_SIZE","G_FILE_ATTRIBUTE_TIME_MODIFIED","G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC
			, G_FILE_QUERY_INFO_NONE, 0, 0);
		if( info ) {
			g_file_info_get_modification_time(info, &(mi->mtime));
			mi->size = g_file_info_get_size(info);
			result = TRUE;
			g_object_unref(info);
		}
		g_object_unref(file);
	}

	mi->need_check = TRUE;
	return result;
}

gboolean doc_close_page( gint page_num );

static gboolean doc_scroll_to_pos( GtkTextView* view ) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextMark* mark = gtk_text_buffer_get_mark(buf, mark_name_view_scroll);

	// move view point to left
	{
		GtkAdjustment* vadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gtk_widget_get_parent(GTK_WIDGET(view))));
		gtk_adjustment_set_value(vadj, 0.0);
	}

	gtk_text_view_scroll_mark_onscreen(view, mark);
	// gtk_text_view_scroll_to_mark(view, mark, 0.0, TRUE, 1.0, 0.25);
	// gtk_text_buffer_delete_mark(buf, mark);

    return FALSE;
}

static void doc_locate_page_line( gint page_num, gint line, gint offset, gboolean need_move_cursor ) {
	GtkTextView* view;
	GtkTextBuffer* buf;
	GtkTextMark* mark;
	GtkTextIter iter;

	if( line <  0 )
		return;

	view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

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

	if( need_move_cursor )
		gtk_text_buffer_place_cursor(buf, &iter);

	mark = gtk_text_buffer_get_mark(buf, mark_name_view_scroll);
	gtk_text_buffer_move_mark(buf, mark, &iter);

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

gboolean doc_cb_button_release_on_view(GtkWidget* widget, GdkEventButton *event) {
	if( event->button==1 )
		puss_pos_locate_add_current_pos();

	return FALSE;
}

gboolean doc_cb_button_release_on_label(GtkWidget* widget, GdkEventButton *event) {
	gint i;
	gint count;

	if( event->button==2 ) {
		count = gtk_notebook_get_n_pages(puss_app->doc_panel);
		for( i=0; i < count; ++i ) {
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
	const Option* font_option;
	const Option* style_option;

	gtk_source_buffer_set_highlight_matching_brackets(buf, TRUE);

	view = GTK_SOURCE_VIEW(puss_text_view_new_with_buffer(buf));
	g_object_unref(G_OBJECT(buf));

	font_option = puss_option_manager_option_find("puss", "editor.font");
	if( font_option && font_option->value) {
		PangoFontDescription* desc = pango_font_description_from_string(font_option->value);
		if( desc ) {
			gtk_widget_modify_font(GTK_WIDGET(view), desc);
			pango_font_description_free(desc);
		}
	}

	style_option = puss_option_manager_option_find("puss", "editor.style");
	if( style_option ) {
		GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
		GtkSourceStyleScheme* style = gtk_source_style_scheme_manager_get_scheme(ssm, style_option->value);
		if( style )
			gtk_source_buffer_set_style_scheme(buf, style);
	}

	label = GTK_LABEL(gtk_label_new(0));
	doc_reset_page_label(GTK_TEXT_BUFFER(buf), label);
	g_signal_connect(buf, "modified-changed", G_CALLBACK(&doc_reset_page_label), label);

	g_signal_connect(view, "button-release-event", G_CALLBACK(&doc_cb_button_release_on_view), 0);

	tab = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(tab), FALSE);
	gtk_container_add(GTK_CONTAINER(tab), GTK_WIDGET(label));
	g_object_set_data(G_OBJECT(tab), "puss-doc-label", label);

	gtk_widget_show_all(tab);

	page = gtk_scrolled_window_new(0, 0);
	g_object_set(page, "border-width", 3, NULL);
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

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buf), &iter);
	gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(buf), mark_name_view_scroll, &iter ,TRUE);

	gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(buf), "puss:searched_mark_start", &iter ,TRUE);
	gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(buf), "puss:searched_mark_end", &iter ,FALSE);

	gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(buf), "puss:searched", "background", "yellow", NULL);
	gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(buf), "puss:searched_current", "background", "green", NULL);

	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(buf), &iter, 0);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buf), &iter);

	return page_num;
}

static gint doc_open_file( const gchar* filename, FindLocation fun, gpointer tag, gboolean show_message_if_open_failed ) {
	ModifyInfo mi;
	GtkTextView* view;
	GtkSourceBuffer* buf;
	GtkTextBuffer* tbuf = 0;
	gchar* url = puss_format_filename(filename);
	gint page_num = puss_doc_find_page_from_url(url);
	gboolean is_new_file = FALSE;
	gboolean need_move_cursor;
	gint line;
	gint offset;

	if( page_num < 0 ) {
		gchar* text = 0;
		gsize len = 0;
		const gchar* charset = 0;
		gboolean BOM = FALSE;

		if( puss_load_file(url, &text, &len, &charset, &BOM) ) {
			// create text buffer & set text
			buf = gtk_source_buffer_new(0);
			tbuf = GTK_TEXT_BUFFER(buf);
			gtk_source_buffer_begin_not_undoable_action(buf);
			gtk_text_buffer_set_text(tbuf, text, len);
			gtk_source_buffer_end_not_undoable_action(buf);
			gtk_text_buffer_set_modified(tbuf, FALSE);
			g_free(text);

			// save url & charset
			puss_doc_set_url(GTK_TEXT_BUFFER(buf), url);
			puss_doc_set_charset(GTK_TEXT_BUFFER(buf), charset);
			puss_doc_set_BOM(GTK_TEXT_BUFFER(buf), BOM);

			// save modify info
			mi.need_check = TRUE;
			if( doc_file_get_mtime(url, &mi) ) {
				g_object_set_data_full(G_OBJECT(buf), "__mi__", g_memdup(&mi, sizeof(mi)), g_free);
			}

			// select highlight language
			gtk_source_buffer_set_language(buf, puss_glob_get_language_by_filename(url));

			// create text view
			page_num = doc_open_page(buf, TRUE);

			is_new_file = TRUE;
		}

	} else {
		view = puss_doc_get_view_from_page_num(page_num);
		if( view ) {
			gtk_notebook_set_current_page(puss_app->doc_panel, page_num);
			gtk_widget_grab_focus(GTK_WIDGET(view));
			tbuf = gtk_text_view_get_buffer(view);
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
	}

	if( tbuf ) {
		need_move_cursor = (*fun)(tbuf, &line, &offset, tag);

		if( is_new_file && line < 0 )
			line = 0;

		if( line >= 0 ) {
			doc_locate_page_line(page_num, line, offset, need_move_cursor);
			puss_pos_locate_add(page_num, line, offset);
		}
	}

	return page_num;
}

/*
gboolean doc_save_file_safe( GtkTextBuffer* buf, GError** err ) {
	ModifyInfo mi;
	const GString* url = puss_doc_get_url(buf);
	gboolean modified = doc_file_get_mtime(url->str, &mi);

	if( modified ) {
		// TODO : ask user if need save as... because of file modified outside!!!
		// BUG : if file modified when check modified finished, then file will be replaced!!!
	}

	return doc_save_file(buf, err);
}
*/

gboolean doc_save_file( GtkTextBuffer* buf, GError** err ) {
	GtkTextIter start, end;
	ModifyInfo mi;

	const GString* url = puss_doc_get_url(buf);
	const GString* charset = puss_doc_get_charset(buf);
	gboolean BOM = puss_doc_get_BOM(buf);
	gchar* text = 0;

	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);
	text = gtk_text_buffer_get_text(buf, &start, &end, TRUE);

	if( !puss_save_file(url->str, text, -1, charset->str, BOM) ) {
		// TODO : message box
		return FALSE;
	}

	gtk_text_buffer_set_modified(buf, FALSE);

	if( doc_file_get_mtime(url->str, &mi) ) {
		ModifyInfo* mi_ptr = (ModifyInfo*)g_object_get_data(G_OBJECT(buf), "__mi__");
		if( mi_ptr )
			*mi_ptr = mi;
		else
			g_object_set_data_full(G_OBJECT(buf), "__mi__", g_memdup(&mi, sizeof(mi)), g_free);
	}

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
	GtkWidget* page;
	GtkWidget* dlg;
	GtkWidget* tab;
	GtkLabel* label;
	gint res;


	if( page_num < 0 )
		return TRUE;

	page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);

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
		dlg = gtk_file_chooser_dialog_new( _("Save File")
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

			tab = gtk_notebook_get_tab_label(puss_app->doc_panel, page);
			label = (GtkLabel*)g_object_get_data(G_OBJECT(tab), "puss-doc-label");
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
	gint res;
	GtkWidget* dlg;
	GtkTextBuffer* buf = puss_doc_get_buffer_from_page_num(page_num);

	if( buf && gtk_text_buffer_get_modified(buf) ) {
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

void puss_doc_set_BOM( GtkTextBuffer* buffer, gboolean BOM ) {
	g_object_set_data(G_OBJECT(buffer), "puss-doc-BOM", GINT_TO_POINTER(BOM));
}

gboolean puss_doc_get_BOM( GtkTextBuffer* buffer ) {
	return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(buffer), "puss-doc-BOM"));
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

gint puss_doc_new() {
	gint page_num;
	GtkSourceBuffer* buf = gtk_source_buffer_new(0);
	puss_doc_set_charset(GTK_TEXT_BUFFER(buf), "UTF-8");
	puss_doc_set_BOM(GTK_TEXT_BUFFER(buf), FALSE);

	puss_pos_locate_add_current_pos();
	page_num = doc_open_page(buf, TRUE);
	puss_pos_locate_add(page_num, 0, 0);

	return page_num;
}

gboolean puss_doc_open_locate( const gchar* url, FindLocation fun, gpointer tag, gboolean show_message_if_open_failed ) {
	gint res;
	GtkWidget* dlg;
	GtkTextBuffer* buffer;
	const GString* gstr;
	gchar* filename;
	gint page_num = -1;
	puss_pos_locate_add_current_pos();

	if( url ) {
		page_num = doc_open_file(url, fun, tag, show_message_if_open_failed);
		if( page_num < 0 )
			return FALSE;

	} else {
		dlg = gtk_file_chooser_dialog_new( "Open File"
			, puss_app->main_window
			, GTK_FILE_CHOOSER_ACTION_OPEN
			, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
			, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT
			, NULL );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

		buffer = doc_get_current_buffer();
		if( buffer ) {
			gstr = puss_doc_get_url(buffer);
			if( gstr ) {
				filename = g_path_get_dirname(gstr->str);
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), filename);
				g_free(filename);
			}
		}

		res = gtk_dialog_run(GTK_DIALOG(dlg));
		if( res == GTK_RESPONSE_ACCEPT ) {
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
			page_num = doc_open_file(filename, fun, tag, show_message_if_open_failed);
			g_free(filename);
		}

		gtk_widget_destroy(dlg);
	}

	return TRUE;
}

typedef struct {
	gint line;
	gint offset;
} DirectLocationTag;

static gboolean doc_direct_locate(GtkTextBuffer* buf, gint* pline, gint* poffset, gpointer tag) {
	*pline = ((DirectLocationTag*)tag)->line;
	*poffset = ((DirectLocationTag*)tag)->offset;
	return TRUE;
}

gboolean puss_doc_open(const gchar* url, gint line, gint line_offset, gboolean show_message_if_open_failed) {
	DirectLocationTag tag = { line, line_offset };
	return puss_doc_open_locate(url, doc_direct_locate, &tag, show_message_if_open_failed);
}

gboolean puss_doc_locate( gint page_num, gint line, gint line_offset, gboolean add_pos_locate ) {
	if( page_num < 0 )
		return FALSE;

	if( add_pos_locate )
		puss_pos_locate_add_current_pos();

	if( line >= 0 ) {
		doc_locate_page_line(page_num, line, line_offset, TRUE);

		if( add_pos_locate )
			puss_pos_locate_add(page_num, line, line_offset);

		return TRUE;
	}

	return FALSE;
}

void puss_doc_save_current( gboolean save_as ) {
	doc_save_page(gtk_notebook_get_current_page(puss_app->doc_panel), save_as);
}

gboolean puss_doc_close_current() {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	return doc_close_page(page_num);
}

void puss_doc_save_all() {
	gint i;
	gint num = gtk_notebook_get_n_pages(puss_app->doc_panel);
	for( i=0; i<num; ++i )
		doc_save_page(i, FALSE);
}

gboolean puss_doc_close_all() {
	GtkWidget* dlg;
	GtkTextBuffer* buf;
	GString* url;
	gint res;

	gboolean need_prompt = TRUE;
	gboolean save_file_sign = FALSE;
	GtkWindow* main_window = puss_app->main_window;
	GtkNotebook* doc_panel = puss_app->doc_panel;
	while( gtk_notebook_get_n_pages(doc_panel) ) {
		buf = puss_doc_get_buffer_from_page_num(0);
		if( buf && gtk_text_buffer_get_modified(buf) ) {
			if( need_prompt ) {
				url = puss_doc_get_url(buf);

				dlg = gtk_message_dialog_new( main_window
					, GTK_DIALOG_MODAL
					, GTK_MESSAGE_QUESTION
					, GTK_BUTTONS_NONE
					, _("file modified, save it?\n\n%s\n")
					, url ? url->str : _("Untitled") );

				gtk_dialog_add_buttons( GTK_DIALOG(dlg)
					, _("yes to all"), 1
					, _("no to all"), 2
					, GTK_STOCK_YES, 3
					, GTK_STOCK_NO, 4
					, NULL );

				gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);

				res = gtk_dialog_run(GTK_DIALOG(dlg));
				gtk_widget_destroy(dlg);

				switch( res ) {
				case 1:
					need_prompt = FALSE;
					save_file_sign = TRUE;
					break;
				case 2:
					need_prompt = FALSE;
					save_file_sign = FALSE;
					break;
				case 3:
					save_file_sign = TRUE;
					break;
				case 4:
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

gboolean doc_mtime_check(gpointer tag) {
	GtkTextBuffer* buf;
	GString* url;
	ModifyInfo* mi_old;
	ModifyInfo mi_new;
	GtkWidget* dlg;
	gint res;
	gchar* text = 0;
	gsize len = 0;
	const gchar* charset = 0;
	gboolean BOM = FALSE;

	if( !gtk_window_is_active(puss_app->main_window) )
		return TRUE;

	buf = doc_get_current_buffer();
	if( !buf )
		return TRUE;

	url = puss_doc_get_url(buf);
	if( !url )
		return TRUE;

	mi_old = (ModifyInfo*)g_object_get_data(G_OBJECT(buf), "__mi__");
	if( !mi_old )
		return TRUE;

	if( !mi_old->need_check )
		return TRUE;

	if( !doc_file_get_mtime(url->str, &mi_new) )
		return TRUE;

	if( mi_old->mtime.tv_sec==mi_new.mtime.tv_sec
		&& mi_old->mtime.tv_usec==mi_new.mtime.tv_usec
		&& mi_old->size==mi_new.size )
	{
		return TRUE;
	}

	// query if use new file!
	{
		dlg = gtk_message_dialog_new( puss_app->main_window
			, GTK_DIALOG_MODAL
			, GTK_MESSAGE_QUESTION
			, GTK_BUTTONS_NONE
			, _("file modified outside, reload it?") );

		gtk_dialog_add_buttons( GTK_DIALOG(dlg)
			, GTK_STOCK_YES, GTK_RESPONSE_YES
			, GTK_STOCK_NO, GTK_RESPONSE_NO
			, _("not tip this"), GTK_RESPONSE_CANCEL
			, NULL );

		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);

		res = gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);

		switch( res ) {
		case GTK_RESPONSE_YES:
			*mi_old = mi_new;
			break;

		case GTK_RESPONSE_NO:
			*mi_old = mi_new;
			return TRUE;

		case GTK_RESPONSE_CANCEL:
			mi_old->need_check = FALSE;
			return TRUE;

		default:
			return TRUE;
		}
	}

	{
		if( puss_load_file(url->str, &text, &len, &charset, &BOM) ) {
			puss_pos_locate_add_current_pos();

			gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(buf));
			gtk_text_buffer_set_text(buf, text, len);
			gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(buf));
			gtk_text_buffer_set_modified(buf, FALSE);
			g_free(text);

			// save charset
			puss_doc_set_charset(GTK_TEXT_BUFFER(buf), charset);
			puss_doc_set_BOM(GTK_TEXT_BUFFER(buf), BOM);

			puss_pos_locate_current();
		}
	}

	return TRUE;
}

gboolean puss_doc_manager_create() {
	g_timeout_add(1000, (GSourceFunc)doc_mtime_check, 0);

	return TRUE;
}

void puss_doc_manager_destroy() {
}

