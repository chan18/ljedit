// Commands.cpp

#include "Commands.h"

#include <glib/gi18n.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"
#include "MiniLine.h"
#include "PosLocate.h"
#include "MiniLineModules.h"
#include "AboutDialog.h"

// file menu

SIGNAL_CALLBACK void cb_file_menu_new( GtkAction* action ) {
	puss_doc_new();
}

SIGNAL_CALLBACK void cb_file_menu_open( GtkAction* action ) {
	puss_doc_open(0, -1, -1);
}

SIGNAL_CALLBACK void cb_file_menu_save( GtkAction* action ) {
	puss_doc_save_current(FALSE);
}

SIGNAL_CALLBACK void cb_file_menu_save_as( GtkAction* action ) {
	puss_doc_save_current(TRUE);
}

SIGNAL_CALLBACK void cb_file_menu_close( GtkAction* action ) {
	puss_doc_close_current();
}

SIGNAL_CALLBACK void cb_file_menu_quit( GtkAction* action ) {
	if( puss_doc_close_all() )
		gtk_widget_destroy(GTK_WIDGET(puss_app->main_window));
}

SIGNAL_CALLBACK void cb_edit_menu_jump_to( GtkAction* action ) {
	puss_mini_line_active(puss_mini_line_GOTO_get_callback());
}

SIGNAL_CALLBACK void cb_edit_menu_find( GtkAction* action ) {
	puss_mini_line_active(puss_mini_line_FIND_get_callback());
}

SIGNAL_CALLBACK void cb_edit_menu_replace( GtkAction* action ) {
	puss_mini_line_active(puss_mini_line_REPLACE_get_callback());
}

SIGNAL_CALLBACK void cb_edit_menu_go_back( GtkAction* action ) {
	puss_pos_locate_back();
}

SIGNAL_CALLBACK void cb_edit_menu_go_forward( GtkAction* action ) {
	puss_pos_locate_forward();
}

SIGNAL_CALLBACK void cb_view_menu_fullscreen( GtkAction* action ) {
	gtk_window_fullscreen(puss_app->main_window);
}

SIGNAL_CALLBACK void cb_view_menu_unfullscreen( GtkAction* action ) {
	gtk_window_unfullscreen(puss_app->main_window);
}

SIGNAL_CALLBACK void cb_view_menu_active_edit_page( GtkAction* action ) {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( !view )
		return;

	gtk_widget_grab_focus(GTK_WIDGET(view));
}

SIGNAL_CALLBACK void cb_view_menu_left_panel( GtkAction* action ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(puss_app->left_panel));
	else
		gtk_widget_hide(GTK_WIDGET(puss_app->left_panel));
}

SIGNAL_CALLBACK void cb_view_menu_right_panel( GtkAction* action ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(puss_app->right_panel));
	else
		gtk_widget_hide(GTK_WIDGET(puss_app->right_panel));
}

SIGNAL_CALLBACK void cb_view_menu_bottom_panel( GtkAction* action ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(puss_app->bottom_panel));
	else
		gtk_widget_hide(GTK_WIDGET(puss_app->bottom_panel));
}

SIGNAL_CALLBACK void cb_view_menu_bottom_page_n( GtkRadioAction* action ) {
	GValue value = { 0 };
	g_value_init(&value, G_TYPE_INT);
	g_object_get_property(G_OBJECT(action), "value", &value);
	gint action_value = g_value_get_int(&value);
	gint page_num = gtk_radio_action_get_current_value(action);

	if( page_num==action_value )
		puss_active_panel_page(puss_app->bottom_panel, (page_num - 1));
}

SIGNAL_CALLBACK void cb_help_menu_about( GtkAction* action ) {
	puss_show_about_dialog(puss_app->main_window);
}

SIGNAL_CALLBACK gboolean cb_main_window_delete(GtkWidget *widget, GdkEvent *event) {
	return !puss_doc_close_all();
}

