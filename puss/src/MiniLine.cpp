// MiniLine.cpp
// 

#include "MiniLine.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "IPuss.h"
#include "Utils.h"
#include "DocManager.h"


struct MiniLine {
	GtkWindow*			window;
	GtkLabel*			label;
	GtkEntry*			entry;

	gulong				signal_id_changed;
	gulong				signal_id_key_press;

	MiniLineCallback*	cb;
};

MiniLine* get_mini_line() {
	static MiniLine me = { 0 };
	return &me;
}

SIGNAL_CALLBACK void mini_line_cb_changed( GtkEditable* editable, Puss* app ) {
	MiniLine* self = get_mini_line();
	if( self->cb ) {
		g_signal_handler_block(G_OBJECT(self->entry), self->signal_id_changed);
		self->cb->cb_changed(app, self->cb->tag);
		g_signal_handler_unblock(G_OBJECT(self->entry), self->signal_id_changed);
	}
}

SIGNAL_CALLBACK gboolean mini_line_cb_key_press_event( GtkWidget* widget, GdkEventKey* event, Puss* app ) {
	MiniLine* self = get_mini_line();
	return self->cb
		? self->cb->cb_key_press(app, event, self->cb->tag)
		: FALSE;
}

SIGNAL_CALLBACK gboolean puss_mini_line_focus_out_event( GtkWidget* widget, GdkEventFocus* event, Puss* app ) {
	puss_mini_line_deactive(app);
	return FALSE;
}

SIGNAL_CALLBACK gboolean mini_line_cb_button_press_event( GtkWidget* widget, GdkEventButton* event, Puss* app ) {
	puss_mini_line_deactive(app);
	return FALSE;
}

void puss_mini_line_create( Puss* app ) {
	MiniLine* self = get_mini_line();

	self->window = GTK_WINDOW(gtk_builder_get_object(app->builder, "mini_window"));
	self->label = GTK_LABEL(gtk_builder_get_object(app->builder, "mini_window_label"));
	self->entry = GTK_ENTRY(gtk_builder_get_object(app->builder, "mini_window_entry"));

	self->signal_id_changed   = g_signal_handler_find(self->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0,&mini_line_cb_changed, app);
	self->signal_id_key_press = g_signal_handler_find(self->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0,&mini_line_cb_key_press_event, app);
}

void puss_mini_line_destroy( Puss* app ) {
	MiniLine* self = get_mini_line();
	if( self->window ) {
		gtk_widget_destroy(GTK_WIDGET(self->window));
		self->window = 0;
	}
}

void puss_mini_line_active( Puss* app, MiniLineCallback* cb ) {
	MiniLine* self = get_mini_line();
	self->cb = cb;
	if( !cb )
		return;

	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(app));
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	GtkWidget* actived = gtk_window_get_focus(puss_get_main_window(app));
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	GdkWindow* gdk_window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	if( !gdk_window )
		return;

	gint x = 0, y = 0;
	gdk_window_get_origin(gdk_window, &x, &y);
	if( x > 16 )	x -= 16;
	if( y > 16 )	y -= 16;

	gtk_window_move(self->window, x, y);
	gtk_widget_show(GTK_WIDGET(self->window));

	// keep cursor both in mini_line_entry & text_view
	// 
	//puss_send_focus_change(GTK_WIDGET(view), FALSE);
	puss_send_focus_change(GTK_WIDGET(self->entry), TRUE);
	//gtk_widget_grab_focus(GTK_WIDGET(self->entry));

	g_signal_handler_block(G_OBJECT(self->entry), self->signal_id_changed);
	gboolean res = self->cb->cb_active(app, self->cb->tag);
	g_signal_handler_unblock(G_OBJECT(self->entry), self->signal_id_changed);
	if( !res )
		puss_mini_line_deactive(app);
}

void puss_mini_line_deactive( Puss* app ) {
	MiniLine* self = get_mini_line();

	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(app));
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);

	gtk_widget_hide(GTK_WIDGET(self->window));

	puss_send_focus_change(GTK_WIDGET(self->entry), FALSE);
	puss_send_focus_change(GTK_WIDGET(view), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(view));
}

