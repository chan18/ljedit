// MiniLine.cpp
// 

#include "MiniLine.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"


struct MiniLineImpl {
	MiniLine			parent;

	MiniLineCallback*	cb;
};

void mini_line_cb_changed( GtkEditable* editable, Puss* app ) {
	MiniLineImpl* self = (MiniLineImpl*)app->mini_line;
	if( self->cb )
		self->cb->cb_changed(app, self->cb->tag);
}

void mini_line_cb_key_press_event( GtkWidget* widget, GdkEventKey* event, Puss* app ) {
	MiniLineImpl* self = (MiniLineImpl*)app->mini_line;
	if( self->cb )
		self->cb->cb_key_press(app, event, self->cb->tag);
}

gboolean puss_mini_line_focus_out_event( GtkWidget* widget, GdkEventFocus* event, Puss* app ) {
	puss_mini_line_deactive(app);
	return FALSE;
}

gboolean mini_line_cb_button_press_event( GtkWidget* widget, GdkEventButton* event, Puss* app ) {
	puss_mini_line_deactive(app);
	return FALSE;
}

void puss_mini_line_create( Puss* app ) {
	app->mini_line = (MiniLine*)g_malloc(sizeof(MiniLineImpl));
	memset(app->mini_line, 0, sizeof(MiniLineImpl));

	MiniLine* ui = app->mini_line;
	
	app->mini_line->label = GTK_LABEL(gtk_label_new(0));

	app->mini_line->entry = GTK_ENTRY(gtk_entry_new());

	g_signal_connect(G_OBJECT(app->mini_line->entry), "changed", (GCallback)&mini_line_cb_changed, app);
	g_signal_connect_after(G_OBJECT(app->mini_line->entry), "key-press-event", (GCallback)&mini_line_cb_key_press_event, app);

	GtkBox* hbox = GTK_BOX(gtk_hbox_new(TRUE, 3));
	gtk_box_pack_start(hbox, GTK_WIDGET(app->mini_line->label), FALSE, FALSE, 0);
	gtk_box_pack_start(hbox, GTK_WIDGET(app->mini_line->entry), TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
	gtk_widget_show_all( GTK_WIDGET(hbox) );

	app->mini_line->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
	gtk_container_add(GTK_CONTAINER(app->mini_line->window), GTK_WIDGET(hbox));

	g_signal_connect_after(G_OBJECT(app->mini_line->window), "focus-out-event", (GCallback)&puss_mini_line_focus_out_event, app);
	g_signal_connect_after(G_OBJECT(app->mini_line->window), "button-press-event", (GCallback)&mini_line_cb_button_press_event, app);

	gtk_window_resize(app->mini_line->window, 200, 24);
	gtk_window_set_modal(app->mini_line->window, TRUE);
}

void puss_mini_line_destroy( Puss* app ) {
	if( app->mini_line ) {
		g_free(app->mini_line);
		app->mini_line = 0;
	}
}

void puss_mini_line_active( Puss* app, gint x, gint y, MiniLineCallback* cb ) {
	MiniLineImpl* self = (MiniLineImpl*)app->mini_line;
	self->cb = cb;
	if( !cb )
		return;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	GtkWidget* actived = gtk_window_get_focus(app->main_window->window);
	if( GTK_WIDGET(view)!=actived )
		return;

	if( !self->cb->cb_active(app, self->cb->tag) ) {
		puss_mini_line_deactive(app);
		return;
	}

	gtk_window_move(app->mini_line->window, x, y);
	gtk_widget_show(GTK_WIDGET(app->mini_line->window));

	puss_send_focus_change(GTK_WIDGET(view), FALSE);
	puss_send_focus_change(GTK_WIDGET(app->mini_line->entry), TRUE);
}

void puss_mini_line_deactive( Puss* app ) {
	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);

	gtk_widget_hide(GTK_WIDGET(app->mini_line->window));

	puss_send_focus_change(GTK_WIDGET(app->mini_line->entry), FALSE);
	puss_send_focus_change(GTK_WIDGET(view), TRUE);
}

