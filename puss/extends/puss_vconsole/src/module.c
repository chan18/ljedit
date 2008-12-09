// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "vconsole.h"

#include "IPuss.h"

#include <libintl.h>

#define TEXT_DOMAIN "puss_ext_vconsole"

#define _(str) dgettext(TEXT_DOMAIN, str)

HMODULE	g_hModule = 0;


#define MAX_SCR_COL	256
#define MAX_SCR_ROW	256
#define MAX_SCR_LEN	(MAX_SCR_COL*MAX_SCR_ROW)

typedef struct {
	Puss*				app;

	GtkWidget*			view;
	gboolean			need_update;

	HMODULE				hDLL;
	TGetVConsoleAPIFn	get_vconsole_api;
	VConsoleAPI*		api;
	VConsole*			vcon;

} PussVConsole;
 
void update_view(PussVConsole* self) {
	VConsole* vcon = self->vcon;
	SMALL_RECT* range;
	DWORD i, j, h, w, sz;
	gchar text[(MAX_SCR_LEN+1)*6 + MAX_SCR_ROW];
	gchar* pd = text;
	CHAR_INFO* ps = vcon->screen_buffer;
	GtkTextIter it, end;
	GtkTextView* txt_view = GTK_TEXT_VIEW(self->view);
	GtkTextBuffer* txt_buf = gtk_text_view_get_buffer(txt_view);

	if( !self->need_update )
		return;
	self->need_update = FALSE;

	range = &(vcon->screen_info->srWindow);
	h = range->Bottom - range->Top + 1;
	w = range->Right - range->Left + 1;
	for( i=0; i<h; ++i ) {
		for( j=0; j<w; ++j ) {
			if( ps->Char.UnicodeChar && g_unichar_validate(ps->Char.UnicodeChar) ) {
				sz = g_unichar_to_utf8(ps->Char.UnicodeChar, pd);
				pd += sz;
				if( ps->Char.UnicodeChar > 0xff )
					++j;
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

	i = vcon->screen_info->dwCursorPosition.Y - range->Top;
	j = vcon->screen_info->dwCursorPosition.X - range->Left;
	h = gtk_text_buffer_get_line_count(txt_buf);
	if( i >= h ) {
		gtk_text_buffer_get_end_iter(txt_buf, &it);
	} else {
		gtk_text_buffer_get_iter_at_line(txt_buf, &it, i);
		end = it;
		if( gtk_text_iter_forward_chars(&end, j) ) {
			it = end;
		} else {
			if( gtk_text_iter_forward_to_line_end(&end) ) {
				it = end;
			} else {
				gtk_text_buffer_get_iter_at_line(txt_buf, &it, i);
			}
		}		
	}
	gtk_text_buffer_place_cursor(txt_buf, &it);
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, VConsole* vcon) {
	switch( event->keyval ) {
	case GDK_Return:
	case GDK_KP_Enter:
		PostMessage(vcon->hwnd, WM_KEYDOWN, 13, 0);
		break;
	case GDK_BackSpace:
		PostMessage(vcon->hwnd, WM_KEYDOWN, 8, 0);
		break;
	case GDK_Control_L:
	case GDK_Control_R:
	case GDK_Shift_L:
	case GDK_Shift_R:
	case GDK_Alt_L:
	case GDK_Alt_R:
		break;
	default:
		PostMessage(vcon->hwnd, WM_CHAR, event->keyval, 0);
	}
	return TRUE;
}

gboolean on_timer(PussVConsole* self) {
	update_view(self);
	return TRUE;
}

void on_screen_changed(VConsole* vcon) {
	PussVConsole* self = (PussVConsole*)vcon->tag;
	self->need_update = TRUE;

	//printf("on_screen_changed\n");
}

void on_quit(VConsole* vcon) {
	PussVConsole* self = (PussVConsole*)vcon->tag;
	//printf("on_quit\n");
	self->need_update = FALSE;

	self->vcon = self->api->create();
	if( self->vcon ) {
		self->vcon->on_screen_changed = on_screen_changed;
		self->vcon->on_quit = on_quit;
		self->vcon->tag = self;
	}
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	PussVConsole* self;
	GtkNotebook* bottom;

	//bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	//bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(PussVConsole, 1);
	self->app = app;
	self->hDLL = LoadLibraryW(L"vconsole.dll");
	if( self->hDLL ) {
		self->get_vconsole_api = (TGetVConsoleAPIFn)GetProcAddress(self->hDLL, "get_vconsole_api");
		self->api = self->get_vconsole_api();

		self->view = gtk_text_view_new();
		gtk_widget_show(self->view);
		bottom = puss_get_bottom_panel(self->app);
		gtk_notebook_append_page(bottom, self->view, gtk_label_new(_("Terminal")));

		self->need_update = FALSE;

		self->vcon = self->api->create();
		if( self->vcon ) {
			self->vcon->on_screen_changed = on_screen_changed;
			self->vcon->on_quit = on_quit;
			self->vcon->tag = self;
		}

		g_signal_connect(self->view, "key-press-event", (GCallback)on_key_press, self);
		g_timeout_add(20, on_timer, self);
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

	if( self->hDLL )
		FreeLibrary(self->hDLL);

	g_free(self);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	g_hModule = hModule;

	switch( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

