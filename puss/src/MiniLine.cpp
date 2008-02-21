// MiniLine.cpp
// 

#include "MiniLine.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "IPuss.h"
#include "Utils.h"
#include "DocManager.h"


struct MiniLineImpl {
	MiniLine			parent;

	gulong				signal_id_changed;
	gulong				signal_id_key_press;
	gulong				signal_id_focus_out;
	gulong				signal_id_button_press;

	MiniLineCallback*	cb;
};

void mini_line_cb_changed( GtkEditable* editable, Puss* app ) {
	MiniLineImpl* self = (MiniLineImpl*)app->mini_line;
	if( self->cb ) {
		g_signal_handler_block(G_OBJECT(app->mini_line->entry), self->signal_id_changed);
		self->cb->cb_changed(app, self->cb->tag);
		g_signal_handler_unblock(G_OBJECT(app->mini_line->entry), self->signal_id_changed);
	}
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
	MiniLineImpl* self = (MiniLineImpl*)g_malloc(sizeof(MiniLineImpl));
	memset(self, 0, sizeof(MiniLineImpl));
	MiniLine* ui = (MiniLine*)self;
	app->mini_line = ui;
	
	ui->label = GTK_LABEL(gtk_label_new(0));
	ui->entry = GTK_ENTRY(gtk_entry_new());

	self->signal_id_changed = g_signal_connect(G_OBJECT(ui->entry), "changed", (GCallback)&mini_line_cb_changed, app);
	self->signal_id_key_press = g_signal_connect(G_OBJECT(ui->entry), "key-press-event", (GCallback)&mini_line_cb_key_press_event, app);

	GtkBox* hbox = GTK_BOX(gtk_hbox_new(FALSE, 0));
	gtk_box_pack_start(hbox, GTK_WIDGET(ui->label), FALSE, FALSE, 0);
	gtk_box_pack_start(hbox, GTK_WIDGET(ui->entry), TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(hbox));
	gtk_widget_show_all(frame);

	ui->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
	gtk_container_add(GTK_CONTAINER(ui->window), frame);

	self->signal_id_focus_out = g_signal_connect(G_OBJECT(ui->window), "focus-out-event", (GCallback)&puss_mini_line_focus_out_event, app);
	self->signal_id_button_press = g_signal_connect(G_OBJECT(ui->window), "button-press-event", (GCallback)&mini_line_cb_button_press_event, app);

	gtk_window_resize(ui->window, 120, 24);
	gtk_window_set_modal(ui->window, TRUE);
}

void puss_mini_line_destroy( Puss* app ) {
	if( app && app->mini_line )
		g_free(app->mini_line);
}

void puss_mini_line_active( Puss* app, MiniLineCallback* cb ) {
	MiniLineImpl* self = (MiniLineImpl*)app->mini_line;
	self->cb = cb;
	if( !cb )
		return;

	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	GtkWidget* actived = gtk_window_get_focus(app->main_window->window);
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	GdkWindow* gdk_window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	if( !gdk_window )
		return;

	gint x = 0, y = 0;
	gdk_window_get_origin(gdk_window, &x, &y);
	if( x > 16 )	x -= 16;
	if( y > 16 )	y -= 16;

	gtk_window_move(app->mini_line->window, x, y);
	gtk_widget_show(GTK_WIDGET(app->mini_line->window));

	// keep cursor both in mini_line_entry & text_view
	// 
	//puss_send_focus_change(GTK_WIDGET(view), FALSE);
	puss_send_focus_change(GTK_WIDGET(app->mini_line->entry), TRUE);
	//gtk_widget_grab_focus(GTK_WIDGET(ui->entry));

	g_signal_handler_block(G_OBJECT(app->mini_line->entry), self->signal_id_changed);
	gboolean res = self->cb->cb_active(app, self->cb->tag);
	g_signal_handler_unblock(G_OBJECT(app->mini_line->entry), self->signal_id_changed);
	if( !res )
		puss_mini_line_deactive(app);
}

void puss_mini_line_deactive( Puss* app ) {
	gint page_num = gtk_notebook_get_current_page(app->main_window->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);

	gtk_widget_hide(GTK_WIDGET(app->mini_line->window));

	puss_send_focus_change(GTK_WIDGET(app->mini_line->entry), FALSE);
	puss_send_focus_change(GTK_WIDGET(view), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(view));
}

