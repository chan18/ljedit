// MiniLine.cpp
// 

#include "MiniLine.h"

#include <glib/gi18n.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"


void mini_line_cb_changed( GtkEditable* editable, Puss* app ) {
	
}

void mini_line_cb_key_press_event( GtkWidget* widget, GdkEventKey* event, Puss* app ) {
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
	MiniLine& ui = app->mini_line;
	
	ui.label = GTK_LABEL(gtk_label_new(0));

	ui.entry = GTK_ENTRY(gtk_entry_new());

	g_signal_connect(G_OBJECT(ui.entry), "changed", (GCallback)&mini_line_cb_changed, app);
	g_signal_connect_after(G_OBJECT(ui.entry), "key-press-event", (GCallback)&mini_line_cb_key_press_event, app);

	GtkBox* vbox = GTK_BOX(gtk_vbox_new(TRUE, 3));
	gtk_box_pack_start(vbox, GTK_WIDGET(ui.label), FALSE, FALSE, 0);
	gtk_box_pack_start(vbox, GTK_WIDGET(ui.entry), TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);
	gtk_widget_show_all( GTK_WIDGET(vbox) );

	ui.window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
	gtk_container_add(GTK_CONTAINER(ui.window), GTK_WIDGET(vbox));

	g_signal_connect_after(G_OBJECT(ui.window), "focus-out-event", (GCallback)&puss_mini_line_focus_out_event, app);
	g_signal_connect_after(G_OBJECT(ui.window), "button-press-event", (GCallback)&mini_line_cb_button_press_event, app);

	gtk_window_resize(ui.window, 200, 24);
	gtk_window_set_modal(ui.window, TRUE);
}

void puss_mini_line_active( Puss* app, CmdLineCallback* cb, gint x, gint y, void* tag ) {
	gint page_num = gtk_notebook_get_current_page(app->main_window.doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	GtkWidget* actived = gtk_window_get_focus(app->main_window.window);
	if( GTK_WIDGET(view)!=actived )
		return;

	if( cb == 0 )
		return;

	if( !cb->cb_active(app, tag) ) {
		puss_mini_line_deactive(app);
		return;
	}

	gtk_window_move(app->mini_line.window, x, y);
	gtk_widget_show(GTK_WIDGET(app->mini_line.window));

	puss_send_focus_change(actived, FALSE);
	puss_send_focus_change(GTK_WIDGET(app->mini_line.entry), TRUE);
}

void puss_mini_line_deactive( Puss* app ) {
	gint page_num = gtk_notebook_get_current_page(app->main_window.doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);

	puss_send_focus_change(GTK_WIDGET(app->mini_line.entry), TRUE);
	puss_send_focus_change(GTK_WIDGET(view), FALSE);
}

