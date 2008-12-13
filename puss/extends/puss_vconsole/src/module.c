// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "vconsole.h"

#include "IPuss.h"

#include <libintl.h>

#define TEXT_DOMAIN "puss_ext_vconsole"

#define _(str) dgettext(TEXT_DOMAIN, str)

#define MAX_SCR_COL	256
#define MAX_SCR_ROW	256
#define MAX_SCR_LEN	(MAX_SCR_COL*MAX_SCR_ROW)

typedef struct {
	Puss*				app;

	gboolean			need_update;
	GtkWidget*			view;
	GtkWidget*			sbar;
	GtkAdjustment*		adjust;

	GModule*			module;
	TGetVConsoleAPIFn	get_vconsole_api;
	VConsoleAPI*		api;
	VConsole*			vcon;

} PussVConsole;
 
void update_view(PussVConsole* self) {
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
				g_print("unknown =  %d\n", ps->Char.UnicodeChar);
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

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, PussVConsole* self) {
	if( self->vcon ) {
		PostMessage(self->vcon->hwnd, WM_KEYDOWN, event->hardware_keycode, 0x001C0001);
		PostMessage(self->vcon->hwnd, WM_KEYUP, event->hardware_keycode, 0xC01C0001);
	}
	return TRUE;
}

void on_im_commit(GtkIMContext *imcontext, gchar *text, PussVConsole* self) {
	gunichar2* utf16_text = g_utf8_to_utf16(text, -1, 0, 0, 0);
	if( utf16_text ) {
		self->api->send_input(self->vcon, utf16_text);
		g_free(utf16_text);
	}
}

gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, PussVConsole* self) {
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

void on_paste(GtkTextView *text_view, PussVConsole* self) {
	GtkClipboard* clipboard = gtk_widget_get_clipboard(self->view, GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_text(clipboard, on_paste_text, self);
}

void on_sbar_changed(GtkAdjustment *adjustment, PussVConsole* self) {
	gdouble value = gtk_adjustment_get_value(adjustment);
	g_print("%f\n", value);

	self->api->scroll(self->vcon, 0, (gint)value);
}

void on_size_allocate(GtkWidget *widget, GtkAllocation *allocation, PussVConsole* self) {
	self->api->resize(self->vcon, 120, allocation->height/18);
}

void on_size_request(GtkWidget *widget, GtkRequisition *requisition, PussVConsole* self) {
	requisition->width = 500;
	requisition->height = 250;
}

gboolean on_timer(PussVConsole* self) {
	update_view(self);
	return TRUE;
}

void vcon_on_screen_changed(VConsole* vcon) {
	PussVConsole* self = (PussVConsole*)vcon->tag;
	self->need_update = TRUE;

	// printf("on_screen_changed\n");
}

void vcon_on_quit(VConsole* vcon) {
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

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	PussVConsole* self;
	GtkNotebook* bottom;
	gchar* vconsole_file;

	//bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	//bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(PussVConsole, 1);
	self->app = app;

	vconsole_file = g_build_filename(app->get_module_path(), "extends", "vconsole.dll", NULL);
	self->module = g_module_open(vconsole_file, G_MODULE_BIND_LAZY);
	g_free(vconsole_file);

	if( self->module ) {
		if( g_module_symbol(self->module, "get_vconsole_api", (gpointer*)&(self->get_vconsole_api)) ) {
			self->api = self->get_vconsole_api();

			self->view = gtk_text_view_new();
			self->adjust = gtk_adjustment_new(0, 0, 14, 1, 14, 14);
			self->sbar = gtk_vscrollbar_new(self->adjust);
			{
				GtkWidget* hbox = gtk_hbox_new(FALSE, 2);
				gtk_box_pack_start(hbox, self->view, TRUE, TRUE, 0);
				gtk_box_pack_start(hbox, self->sbar, FALSE, FALSE, 0);
				gtk_widget_show_all(hbox);

				bottom = puss_get_bottom_panel(self->app);
				gtk_notebook_append_page(bottom, hbox, gtk_label_new(_("Terminal")));
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
			g_timeout_add(20, on_timer, self);
		}
	}

	return self;
}

PUSS_EXPORT void  puss_extend_destroy(void* ext) {
	PussVConsole* self = (PussVConsole*)ext;

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

