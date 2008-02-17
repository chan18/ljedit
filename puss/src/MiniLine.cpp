// MiniLine.cpp
// 

#include "MiniLine.h"
#include "Puss.h"

#include <glib/gi18n.h>

void mini_line_cb_changed( Puss* app ) {
	
}

void mini_line_cb_key_press( Puss* app ) {
}

void puss_mini_line_create( Puss* app ) {
	MiniLine& ui = app->mini_line;
	
	ui.label = GTK_LABEL(gtk_label_new(0));

	ui.entry = GTK_ENTRY(gtk_entry_new());

	g_signal_connect(G_OBJECT(ui.entry), "changed", (GCallback)&mini_line_cb_changed, app);
	g_signal_connect_after(G_OBJECT(ui.entry), "key-press-event", (GCallback)&mini_line_cb_key_press, app);

	GtkBox* vbox = GTK_BOX(gtk_vbox_new(TRUE, 3));
	gtk_box_pack_start(vbox, GTK_WIDGET(ui.label), FALSE, FALSE, 0);
	gtk_box_pack_start(vbox, GTK_WIDGET(ui.entry), TRUE, TRUE, 0);

	ui.window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
	//g_signal_connect_after(G_OBJECT(window), "button-press-event", (GCallback)&xxxx, app);

	// TODO : continue
}

void puss_mini_line_active( Puss* app, CmdLineCallback* cb, gint x, gint y, void* tag ) {
}

void puss_mini_line_deactive( Puss* app ) {
}

