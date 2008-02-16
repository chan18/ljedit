// CmdLine.cpp
// 

#include "CmdLine.h"
#include "Puss.h"

#include <glib/gi18n.h>

void cmd_line_cb_changed( Puss* app ) {
	
}

void cmd_line_cb_key_press( Puss* app ) {
}

GtkLabel* puss_cmd_line_get_label( GtkWidget* cmd_line ) {
	return 0;
}

GtkEntry* puss_cmd_line_get_entry( GtkWidget* cmd_line ) {
	return 0;
}

void puss_cmd_line_create( Puss* app ) {
	GtkLabel* label = GTK_LABEL(gtk_label_new(0));

	GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
	g_signal_connect(G_OBJECT(entry), "changed", (GCallback)&cmd_line_cb_changed, app);
	g_signal_connect_after(G_OBJECT(entry), "key-press", (GCallback)&cmd_line_cb_key_press, app);

	GtkBox* vbox = GTK_BOX(gtk_vbox_new(TRUE, 3));
	gtk_box_pack_start(vbox, GTK_WIDGET(label), FALSE, FALSE, 0);
	gtk_box_pack_start(vbox, GTK_WIDGET(entry), TRUE, TRUE, 0);

	GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
	//g_signal_connect_after(G_OBJECT(window), "button-press-event", (GCallback)&xxxx, app);

	// TODO : continue
	app->ui.cmd_line_window = window;
}

void puss_cmd_line_destroy( Puss* app );

void puss_cmd_line_active( Puss* app, CmdLineCallback* cb, gint x, gint y, void* tag ) {
}

void puss_cmd_line_deactive( Puss* app ) {
}

