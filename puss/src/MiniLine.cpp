// MiniLine.cpp
// 

#include "MiniLine.h"

#include <glib/gi18n.h>
#include <memory.h>
#include <stdlib.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"

SIGNAL_CALLBACK void mini_line_cb_changed( GtkEditable* editable ) {
	if( puss_app->mini_line->cb ) {
		g_signal_handler_block(G_OBJECT(puss_app->mini_line->entry), puss_app->mini_line->signal_id_changed);
		puss_app->mini_line->cb->cb_changed(puss_app->mini_line->cb->tag);
		g_signal_handler_unblock(G_OBJECT(puss_app->mini_line->entry), puss_app->mini_line->signal_id_changed);
	}
}

SIGNAL_CALLBACK gboolean mini_line_cb_key_press_event( GtkWidget* widget, GdkEventKey* event ) {
	return puss_app->mini_line->cb
		? puss_app->mini_line->cb->cb_key_press(event, puss_app->mini_line->cb->tag)
		: FALSE;
}

SIGNAL_CALLBACK gboolean puss_mini_line_focus_out_event( GtkWidget* widget, GdkEventFocus* event ) {
	puss_mini_line_deactive();
	return FALSE;
}

SIGNAL_CALLBACK gboolean mini_line_cb_button_press_event( GtkWidget* widget, GdkEventButton* event ) {
	puss_mini_line_deactive();
	return FALSE;
}

void puss_mini_line_create() {
	g_assert( !puss_app->mini_line );

	puss_app->mini_line = g_new(MiniLine, 1);
	if( !puss_app->mini_line ) {
		g_printerr("ERROR : new mini line failed!\n");
		exit(1);
	}

	puss_app->mini_line->window = GTK_WINDOW(gtk_builder_get_object(puss_app->builder, "mini_window"));
	puss_app->mini_line->label = GTK_LABEL(gtk_builder_get_object(puss_app->builder, "mini_window_label"));
	puss_app->mini_line->entry = GTK_ENTRY(gtk_builder_get_object(puss_app->builder, "mini_window_entry"));

	puss_app->mini_line->signal_id_changed   = g_signal_handler_find(puss_app->mini_line->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0,&mini_line_cb_changed, 0);
	puss_app->mini_line->signal_id_key_press = g_signal_handler_find(puss_app->mini_line->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0,&mini_line_cb_key_press_event, 0);
}

void puss_mini_line_destroy() {
	if( puss_app->mini_line->window )
		gtk_widget_destroy(GTK_WIDGET(puss_app->mini_line->window));

	g_free(puss_app->mini_line);
	puss_app->mini_line = 0;
}

void puss_mini_line_active( MiniLineCallback* cb ) {
	puss_app->mini_line->cb = cb;
	if( !cb )
		return;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	GtkWidget* actived = gtk_window_get_focus(puss_app->main_window);
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	GdkWindow* gdk_window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	if( !gdk_window )
		return;

	gint x = 0, y = 0;
	gdk_window_get_origin(gdk_window, &x, &y);
	if( x > 16 )	x -= 16;
	if( y > 16 )	y -= 16;

	gtk_window_move(puss_app->mini_line->window, x, y);
	gtk_widget_show(GTK_WIDGET(puss_app->mini_line->window));

	// keep cursor both in mini_line_entry & text_view
	// 
	//puss_send_focus_change(GTK_WIDGET(view), FALSE);
	puss_send_focus_change(GTK_WIDGET(puss_app->mini_line->entry), TRUE);
	//gtk_widget_grab_focus(GTK_WIDGET(puss_app->mini_line->entry));

	g_signal_handler_block(G_OBJECT(puss_app->mini_line->entry), puss_app->mini_line->signal_id_changed);
	gboolean res = puss_app->mini_line->cb->cb_active(puss_app->mini_line->cb->tag);
	g_signal_handler_unblock(G_OBJECT(puss_app->mini_line->entry), puss_app->mini_line->signal_id_changed);
	if( !res )
		puss_mini_line_deactive();
}

void puss_mini_line_deactive() {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);

	gtk_widget_hide(GTK_WIDGET(puss_app->mini_line->window));

	puss_send_focus_change(GTK_WIDGET(puss_app->mini_line->entry), FALSE);
	puss_send_focus_change(GTK_WIDGET(view), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(view));
}

