// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "IPuss.h"

#include <libintl.h>

#define TEXT_DOMAIN "plugin_vconsole"

#define _(str) dgettext(TEXT_DOMAIN, str)

#ifdef G_OS_WIN32

//-----------------------------------------------------------------

#include "vconsole.h"

#define MAX_SCR_COL	256
#define MAX_SCR_ROW	256
#define MAX_SCR_LEN	(MAX_SCR_COL*MAX_SCR_ROW)

typedef struct {
	Puss*				app;

	GtkWidget*			panel;

	gboolean			need_update;
	GtkWidget*			view;
	GtkWidget*			sbar;
	GtkAdjustment*		adjust;

	GModule*			module;
	TGetVConsoleAPIFn	get_vconsole_api;
	VConsoleAPI*		api;
	VConsole*			vcon;

} PussVConsole;
 
static void update_view(PussVConsole* self) {
	VConsole* vcon = self->vcon;
	SMALL_RECT* range;
	DWORD i, j, h, w, sz;
	DWORD cx, cy;
	gchar text[(MAX_SCR_LEN+1)*6 + MAX_SCR_ROW];
	gchar* pd = text;
	CHAR_INFO* ps;
	GtkTextIter it, end;
	GtkTextView* txt_view = GTK_TEXT_VIEW(self->view);
	GtkTextBuffer* txt_buf = gtk_text_view_get_buffer(txt_view);

	if( !self->vcon || !self->need_update )
		return;
	self->need_update = FALSE;

	range = &(vcon->screen_info->srWindow);
	h = range->Bottom - range->Top + 1;
	w = range->Right - range->Left + 1;
	cy = vcon->screen_info->dwCursorPosition.Y - range->Top;
	cx = vcon->screen_info->dwCursorPosition.X - range->Left;

	// scroll bar update
	self->adjust->lower = 1;
	self->adjust->upper = vcon->screen_info->dwSize.Y;
	self->adjust->step_increment = 1;
	self->adjust->page_size = h;
	self->adjust->page_increment = h;
	self->adjust->value = range->Top;

	gtk_adjustment_changed(self->adjust);

	// screen buffer update
	ps = vcon->screen_buffer;
	for( i=0; i<h; ++i ) {
		for( j=0; j<w; ++j ) {
			if( ps->Char.UnicodeChar && g_unichar_validate(ps->Char.UnicodeChar) ) {
				sz = g_unichar_to_utf8(ps->Char.UnicodeChar, pd);
				pd += sz;
				if( ps->Char.UnicodeChar > 0xff ) {
					++j;
					if( cy==i )
						--cx;
				}
			} else {
				//g_print("unknown =  %d\n", ps->Char.UnicodeChar);
			}
			++ps;
		}
		*pd = '\n';
		++pd;
	}
	*pd = '\0';
	sz = (DWORD)(pd - text);

	gtk_text_buffer_set_text(txt_buf, text, sz);

	// cursor update
	h = gtk_text_buffer_get_line_count(txt_buf);
	if( cy >= h ) {
		gtk_text_view_set_cursor_visible(txt_view, FALSE);

	} else {
		gtk_text_buffer_get_iter_at_line(txt_buf, &it, cy);
		end = it;
		if( gtk_text_iter_forward_chars(&end, cx) ) {
			it = end;
		} else {
			if( gtk_text_iter_forward_to_line_end(&end) ) {
				it = end;
			} else {
				gtk_text_buffer_get_iter_at_line(txt_buf, &it, cy);
			}
		}
		gtk_text_buffer_place_cursor(txt_buf, &it);
		gtk_text_view_set_cursor_visible(txt_view, TRUE);
	}
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, PussVConsole* self) {
	if( self->vcon ) {
		PostMessage(self->vcon->hwnd, WM_KEYDOWN, event->hardware_keycode, 0x001C0001);
		PostMessage(self->vcon->hwnd, WM_KEYUP, event->hardware_keycode, 0xC01C0001);
	}
	return TRUE;
}

static void on_im_commit(GtkIMContext *imcontext, gchar *text, PussVConsole* self) {
	gunichar2* utf16_text = g_utf8_to_utf16(text, -1, 0, 0, 0);
	if( utf16_text ) {
		self->api->send_input(self->vcon, utf16_text);
		g_free(utf16_text);
	}
}

static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, PussVConsole* self) {
	gdouble y_new = self->adjust->value;
	switch( event->direction ) {
	case GDK_SCROLL_UP:
		y_new -= 3;
		if( y_new < self->adjust->lower )
			y_new = self->adjust->lower;
		break;
	case GDK_SCROLL_DOWN:
		y_new += 3;
		if( y_new > self->adjust->upper )
			y_new = self->adjust->upper;
		break;
	case GDK_SCROLL_LEFT:
	case GDK_SCROLL_RIGHT:
		break;
	}

	if( y_new != self->adjust->value ) {
		self->adjust->value = y_new;
		gtk_adjustment_value_changed(self->adjust);
	}

	return TRUE;
}

static void on_paste_text(GtkClipboard *clipboard, const gchar *text, PussVConsole* self)
{
	gunichar2* utf16_text = g_utf8_to_utf16(text, -1, 0, 0, 0);
	if( utf16_text ) {
		self->api->send_input(self->vcon, utf16_text);
		g_free(utf16_text);
	}
}

static void on_paste(GtkTextView *text_view, PussVConsole* self) {
	GtkClipboard* clipboard = gtk_widget_get_clipboard(self->view, GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_text(clipboard, on_paste_text, self);
}

static void on_sbar_changed(GtkAdjustment *adjustment, PussVConsole* self) {
	gdouble value = gtk_adjustment_get_value(adjustment);
	self->api->scroll(self->vcon, 0, (gint)value);
}

static void on_size_allocate(GtkWidget *widget, GtkAllocation *allocation, PussVConsole* self) {
	gint h;
	GtkTextIter iter;
	GdkRectangle loc;
	GtkTextView* view = GTK_TEXT_VIEW( self->view );
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);

	gtk_text_buffer_get_start_iter(buf, &iter);
	gtk_text_view_get_iter_location(view, &iter, &loc);
	h = loc.height;
	h += gtk_text_view_get_pixels_below_lines(view);
	h += gtk_text_view_get_pixels_above_lines(view);

	self->api->resize(self->vcon, 120, allocation->height/h);
}

static void on_size_request(GtkWidget *widget, GtkRequisition *requisition, PussVConsole* self) {
	requisition->width = 500;
	requisition->height = 1;
}

static void on_reset_btn_click(GtkButton *button, PussVConsole* self) {
	self->api->destroy(self->vcon);
}

static gboolean on_show_console(PussVConsole* self) {
	RECT rect;
	if( self->vcon ) {
		ShowWindow(self->vcon->hwnd, SW_SHOW);
		GetWindowRect(self->vcon->hwnd, &rect);
		MoveWindow(self->vcon->hwnd, 100, 100, rect.right - rect.left, rect.bottom - rect.left, FALSE);
		BringWindowToTop(self->vcon->hwnd);
	}
	return FALSE;
}

static void on_show_hide_btn_click(GtkButton *button, PussVConsole* self) {
	if( self->vcon ) {
		if( IsWindowVisible(self->vcon->hwnd) ) {
			ShowWindow(self->vcon->hwnd, SW_HIDE);
		} else {
			g_idle_add(on_show_console, self);
		}
	}
}

static gboolean on_active(GtkWidget* widget, GdkEventFocus* event, PussVConsole* self) {
	gtk_widget_grab_focus(self->view);
	return TRUE;
}

static gboolean on_idle_update(PussVConsole* self) {
	update_view(self);
	return FALSE;
}

static void vcon_on_screen_changed(VConsole* vcon) {
	PussVConsole* self = (PussVConsole*)vcon->tag;
	self->need_update = TRUE;

	g_idle_add(on_idle_update, self);	// it's thread safe
}

static void vcon_on_quit(VConsole* vcon) {
	PussVConsole* self = (PussVConsole*)vcon->tag;
	//printf("on_quit\n");
	self->need_update = FALSE;

	self->vcon = self->api->create();
	if( self->vcon ) {
		self->vcon->on_screen_changed = vcon_on_screen_changed;
		self->vcon->on_quit = vcon_on_quit;
		self->vcon->tag = self;
	}
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	PussVConsole* self;
	gchar* vconsole_file;
	PangoFontDescription* desc;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(PussVConsole, 1);
	self->app = app;

	vconsole_file = g_build_filename(app->get_plugins_path(), "vconsole.dll", NULL);
	self->module = g_module_open(vconsole_file, G_MODULE_BIND_LAZY);
	g_free(vconsole_file);

	if( self->module ) {
		if( g_module_symbol(self->module, "get_vconsole_api", (gpointer*)&(self->get_vconsole_api)) ) {
			self->api = self->get_vconsole_api();

			self->view = gtk_text_view_new();
			desc = pango_font_description_from_string("monospace 9");
			if( desc ) {
				gtk_widget_modify_font(self->view, desc);
				pango_font_description_free(desc);
			}

			self->adjust = GTK_ADJUSTMENT( gtk_adjustment_new(0, 0, 14, 1, 14, 14) );
			self->sbar = gtk_vscrollbar_new(self->adjust);
			{
				GtkWidget* reset_btn = gtk_button_new_with_label("reset");
				GtkWidget* show_hide_btn = gtk_button_new_with_label("show/hide");
				GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
				GtkWidget* hbox = gtk_hbox_new(FALSE, 2);

				g_signal_connect(reset_btn, "clicked", (GCallback)on_reset_btn_click, self);
				g_signal_connect(show_hide_btn, "clicked", (GCallback)on_show_hide_btn_click, self);

				g_signal_connect(hbox, "focus-in-event",G_CALLBACK(&on_active), self);

				gtk_box_pack_start(GTK_BOX(vbox), reset_btn, FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX(vbox), show_hide_btn, FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX(hbox), self->view, TRUE, TRUE, 0);
				gtk_box_pack_start(GTK_BOX(hbox), self->sbar, FALSE, FALSE, 0);
				gtk_widget_show_all(hbox);

				self->panel = hbox;
				self->app->panel_append(self->panel, gtk_label_new(_("Terminal")), "puss_vconsole_plugin_panel", PUSS_PANEL_POS_BOTTOM);
			}

			self->need_update = FALSE;

			self->vcon = self->api->create();
			if( self->vcon ) {
				self->vcon->on_screen_changed = vcon_on_screen_changed;
				self->vcon->on_quit = vcon_on_quit;
				self->vcon->tag = self;
			}

			g_signal_connect(self->view, "key-press-event", (GCallback)on_key_press, self);
			g_signal_connect(self->view, "scroll-event", (GCallback)on_scroll_event, self);
			g_signal_connect(self->view, "paste-clipboard", (GCallback)on_paste, self);
			g_signal_connect(self->view, "size-allocate", (GCallback)on_size_allocate, self);
			g_signal_connect(self->view, "size-request", (GCallback)on_size_request, self);
			g_signal_connect(GTK_TEXT_VIEW(self->view)->im_context, "commit", (GCallback)on_im_commit, self);
			g_signal_connect(self->adjust, "value-changed", (GCallback)on_sbar_changed, self);
		}
	}

	return self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* ext) {
	PussVConsole* self = (PussVConsole*)ext;

	self->app->panel_remove(self->panel);

	if( self->api ) {
		if( self->vcon ) {
			self->vcon->on_quit = 0;
			self->vcon->on_screen_changed = 0;
			self->api->destroy(self->vcon);
		}
	}

	if( self->module )
		g_module_close(self->module);

	g_free(self);
}


#else//LINUX IMPLEMENTS

//-----------------------------------------------------------------

#include <vte/vte.h>

typedef struct {
	Puss*		app;
	GtkWidget*	vte;
} PussVConsole;

static void on_quit() {
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	PussVConsole* self;
	GtkWidget* bottom;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(PussVConsole, 1);
	self->app = app;
	self->vte = vte_terminal_new();

	{
		GtkWidget* hbox = gtk_hbox_new(FALSE, 4);
		GtkWidget* sbar = gtk_vscrollbar_new( vte_terminal_get_adjustment(self->vte) );
        
		/*
        gtk.HBox.__init__(self, False, 4)

        gconf_client.add_dir(self.GCONF_PROFILE_DIR,
                             gconf.CLIENT_PRELOAD_RECURSIVE)

        self._vte = vte.Terminal()
        self.reconfigure_vte()
        self._vte.set_size(self._vte.get_column_count(), 5)
        self._vte.set_size_request(200, 50)
        self._vte.show()

        gconf_client.notify_add(self.GCONF_PROFILE_DIR,
                                self.on_gconf_notification)
        self._vte.connect("key-press-event", self.on_vte_key_press)
        self._vte.connect("child-exited", lambda term: term.fork_command())

        self._vte.fork_command()
        */
        
        gtk_widget_show_all(hbox);

		bottom = puss_get_bottom_panel(app);
		gtk_notebook_append_page(GTK_NOTEBOOK(bottom), hbox, gtk_label_new(_("Terminal")));

		gtk_box_pack_start(GTK_BOX(hbox), self->vte, TRUE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), sbar, FALSE, FALSE, 0);

		g_signal_connect(self->vte, "child-exited", on_quit, self);

		vte_terminal_fork_command( VTE_TERMINAL(self->vte)
				, 0, 0
				, 0, 0
				, FALSE
				, FALSE
				, FALSE );
	}

	return self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* ext) {
	PussVConsole* self = (PussVConsole*)ext;
	g_free(self);
}

#endif
