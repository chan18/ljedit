// MiniLine.cpp
// 

#include "MiniLine.h"

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

SIGNAL_CALLBACK gboolean mini_line_cb_button_press_event( GtkWidget* widget, GdkEventButton* event ) {
	puss_mini_line_deactive();
	return FALSE;
}

gboolean puss_mini_line_create() {
	g_assert( !puss_app->mini_line );

	puss_app->mini_line = g_new0(MiniLine, 1);

	puss_app->mini_line->window = GTK_WIDGET(gtk_builder_get_object(puss_app->builder, "mini_bar_window"));
	puss_app->mini_line->image = GTK_IMAGE(gtk_builder_get_object(puss_app->builder, "mini_bar_image"));
	puss_app->mini_line->entry = GTK_ENTRY(gtk_builder_get_object(puss_app->builder, "mini_bar_entry"));
	gtk_widget_hide(puss_app->mini_line->window);

	if( !( puss_app->mini_line->image
		&& puss_app->mini_line->entry ) )
	{
		return FALSE;
	}

	puss_app->mini_line->signal_id_changed   = g_signal_handler_find(puss_app->mini_line->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0, (gpointer)&mini_line_cb_changed, 0);
	puss_app->mini_line->signal_id_key_press = g_signal_handler_find(puss_app->mini_line->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0, (gpointer)&mini_line_cb_key_press_event, 0);

	return TRUE;
}

void puss_mini_line_destroy() {
	g_free(puss_app->mini_line);
	puss_app->mini_line = 0;
}

void puss_mini_line_active( MiniLineCallback* cb ) {
	puss_app->mini_line->cb = cb;
	if( !cb )
		return;

	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	if( page_num < 0 )
		return;

	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	GtkWidget* actived = gtk_window_get_focus(puss_app->main_window);
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	gtk_widget_show(puss_app->mini_line->window);
	gtk_im_context_focus_out( view->im_context );

	gtk_widget_grab_focus(GTK_WIDGET(puss_app->mini_line->entry));

	g_signal_handler_block(G_OBJECT(puss_app->mini_line->entry), puss_app->mini_line->signal_id_changed);
	gboolean res = puss_app->mini_line->cb->cb_active(puss_app->mini_line->cb->tag);
	g_signal_handler_unblock(G_OBJECT(puss_app->mini_line->entry), puss_app->mini_line->signal_id_changed);
	if( !res )
		puss_mini_line_deactive();
}

void puss_mini_line_deactive() {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);

	gtk_widget_hide(puss_app->mini_line->window);

	gtk_widget_grab_focus(GTK_WIDGET(view));
}

