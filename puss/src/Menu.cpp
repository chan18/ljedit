// Menu.cpp

#include "Menu.h"

#include <glib/gi18n.h>

#include "IPuss.h"
#include "Utils.h"
#include "DocManager.h"
#include "MiniLine.h"
#include "PosLocate.h"
#include "MiniLineModules.h"
#include "AboutDialog.h"

// file menu

PUSS_EXPORT void cb_file_menu_new( GtkAction* action, Puss* app ) {
	puss_doc_new(app);
}

PUSS_EXPORT void cb_file_menu_open( GtkAction* action, Puss* app ) {
	puss_doc_open(app, 0, 0, 0);
}

PUSS_EXPORT void cb_file_menu_save( GtkAction* action, Puss* app ) {
	puss_doc_save_current(app, FALSE);
}

PUSS_EXPORT void cb_file_menu_save_as( GtkAction* action, Puss* app ) {
	puss_doc_save_current(app, TRUE);
}

PUSS_EXPORT void cb_file_menu_close( GtkAction* action, Puss* app ) {
	puss_doc_close_current(app);
}

PUSS_EXPORT void cb_file_menu_quit( GtkAction* action, Puss* app ) {
	gtk_main_quit();
}

PUSS_EXPORT void cb_edit_menu_jump_to( GtkAction* action, Puss* app ) {
	puss_mini_line_active(app, puss_mini_line_GOTO_get_callback());
}

PUSS_EXPORT void cb_edit_menu_find( GtkAction* action, Puss* app ) {
	puss_mini_line_active(app, puss_mini_line_FIND_get_callback());
}

PUSS_EXPORT void cb_edit_menu_replace( GtkAction* action, Puss* app ) {
	puss_mini_line_active(app, puss_mini_line_REPLACE_get_callback());
}

PUSS_EXPORT void cb_edit_menu_go_back( GtkAction* action, Puss* app ) {
	puss_pos_locate_back(app);
}

PUSS_EXPORT void cb_edit_menu_go_forward( GtkAction* action, Puss* app ) {
	puss_pos_locate_forward(app);
}

PUSS_EXPORT void cb_view_menu_fullscreen( GtkAction* action, Puss* app ) {
	static bool is_fullscreen = false;
	is_fullscreen = !is_fullscreen;

	GtkWindow* main_window = puss_get_main_window(app);

	if( is_fullscreen )
		gtk_window_fullscreen(main_window);
	else
		gtk_window_unfullscreen(main_window);
}

PUSS_EXPORT void cb_view_menu_active_doc_page( GtkAction* action, Puss* app ) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(app));
	GtkTextView* view = puss_doc_get_view_from_page_num(app, page_num);
	if( !view )
		return;

	gtk_widget_grab_focus(GTK_WIDGET(view));
}

PUSS_EXPORT void cb_view_menu_left_panel( GtkAction* action, Puss* app ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(puss_get_left_panel(app)));
	else
		gtk_widget_hide(GTK_WIDGET(puss_get_left_panel(app)));
}

PUSS_EXPORT void cb_view_menu_right_panel( GtkAction* action, Puss* app ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(puss_get_right_panel(app)));
	else
		gtk_widget_hide(GTK_WIDGET(puss_get_right_panel(app)));
}

PUSS_EXPORT void cb_view_menu_bottom_panel( GtkAction* action, Puss* app ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(puss_get_bottom_panel(app)));
	else
		gtk_widget_hide(GTK_WIDGET(puss_get_bottom_panel(app)));
}

PUSS_EXPORT void cb_view_menu_bottom_page_n( GtkAction* action, GtkRadioAction* current, Puss* app ) {
	gint page_num = gtk_radio_action_get_current_value(current);
	puss_active_panel_page(puss_get_bottom_panel(app), (page_num - 1));
}

PUSS_EXPORT void cb_help_menu_about( GtkAction* action, Puss* app ) {
	puss_show_about_dialog(puss_get_main_window(app));
}


