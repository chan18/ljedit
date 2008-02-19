// Menu.cpp

#include "Menu.h"

#include <glib/gi18n.h>

#include "Puss.h"
#include "DocManager.h"
#include "MiniLineModules.h"
#include "AboutDialog.h"

// file menu
void cb_file_menu_new( GtkAction* action, Puss* app );
void cb_file_menu_open( GtkAction* action, Puss* app );
void cb_file_menu_save( GtkAction* action, Puss* app );
void cb_file_menu_save_as( GtkAction* action, Puss* app );
void cb_file_menu_close( GtkAction* action, Puss* app );
void cb_file_menu_quit( GtkAction* action, Puss* app );

// edit menu
void cb_edit_menu_goto( GtkAction* action, Puss* app );
void cb_edit_menu_find( GtkAction* action, Puss* app );
void cb_edit_menu_replace( GtkAction* action, Puss* app );
void cb_edit_menu_go_back( GtkAction* action, Puss* app );
void cb_edit_menu_go_forward( GtkAction* action, Puss* app );

// view menu
void cb_view_menu_fullscreen( GtkAction* action, Puss* app );

void cb_view_menu_active_doc_page( GtkAction* action, Puss* app );

void cb_view_menu_left_panel( GtkAction* action, Puss* app );
void cb_view_menu_right_panel( GtkAction* action, Puss* app );
void cb_view_menu_bottom_panel( GtkAction* action, Puss* app );

void cb_view_menu_bottom_page_n( GtkAction* action, GtkRadioAction* current, Puss* app );

// help menu
void cb_help_menu_about( GtkAction* action, Puss* app );

void puss_create_ui_manager(Puss* app) {
	GError* error = 0;
	GtkActionGroup* action_group = gtk_action_group_new("LJEditActions");

	{
		static GtkActionEntry entries[] = {
			// main menu
			  { "FileMenu",    0, _("_File") }
			, { "EditMenu",    0, _("_Edit") }
			, { "ViewMenu",    0, _("_View") }
			, { "ToolsMenu",   0, _("_Tools") }
			, { "PluginsMenu", 0, _("_Plugins") }
			, { "HelpMenu",    0, _("_Help") }

			// file menu
			, { "New",    GTK_STOCK_NEW,     _("_New"),         "<control>N",        _("create new file"),    G_CALLBACK(&cb_file_menu_new) }
			, { "Open",   GTK_STOCK_OPEN,    _("_Open"),        "<control>O",        _("open file"),          G_CALLBACK(&cb_file_menu_open) }
			, { "Save",   GTK_STOCK_SAVE,    _("_Save"),        "<control>S",        _("save file"),          G_CALLBACK(&cb_file_menu_save) }
			, { "SaveAs", GTK_STOCK_SAVE_AS, _("Save _As ..."), "<control><shift>S", _("save file as..."),    G_CALLBACK(&cb_file_menu_save_as) }
			, { "Close",  GTK_STOCK_CLOSE,   _("_Close"),       "<control>W",        _("close current file"), G_CALLBACK(&cb_file_menu_close) }
			, { "Quit",   GTK_STOCK_QUIT,    _("_Quit"),        "<control>Q",        _("quit"),               G_CALLBACK(&cb_file_menu_quit) }

			// edit menu
			, { "CmdLineGoto",    GTK_STOCK_JUMP_TO,          _("_Goto"),         "<control>I", _("active goto command line"),    G_CALLBACK(&cb_edit_menu_goto) }
			, { "CmdLineFind",    GTK_STOCK_FILE,             _("_Find"),         "<control>K", _("active find command line"),    G_CALLBACK(&cb_edit_menu_find) }
			, { "CmdLineReplace", GTK_STOCK_FIND_AND_REPLACE, _("_Replace"),      "<control>H", _("active replace command line"), G_CALLBACK(&cb_edit_menu_replace) }
			, { "GoBack",         GTK_STOCK_GO_BACK,          _("Go_Back"),       "<alt>Z",     _("go back position"),            G_CALLBACK(&cb_edit_menu_go_back) }
			, { "GoForward",      GTK_STOCK_GO_FORWARD,       _("Go_Forward"),    "<alt>X",     _("go forward position"),         G_CALLBACK(&cb_edit_menu_go_forward) }

			// view menu
			, { "FullScreen",     GTK_STOCK_FULLSCREEN,       _("FullScreen"),    "F11",        _("set/unset fullscreen"),        G_CALLBACK(&cb_view_menu_fullscreen) }

			, { "ActiveDocPage",  0,                          _("ActiveDocPage"), "<control>0", _("active document page"),        G_CALLBACK(&cb_view_menu_active_doc_page) }

			, { "BottomPages",    GTK_STOCK_INDEX,            _("BottomPages") }

			// help menu
			, { "About",          GTK_STOCK_ABOUT,            _("About"),         0,            _("about ..."),                   G_CALLBACK(&cb_help_menu_about) }
		};
		gtk_action_group_add_actions(action_group, entries, G_N_ELEMENTS(entries), app);
	}

	{
		static GtkToggleActionEntry entries[] = {
			  { "LeftPanel",     GTK_STOCK_INFO,       _("LeftPanel"),   0,     _("show/hide left panel"),   G_CALLBACK(&cb_view_menu_left_panel),   TRUE }
			, { "RightPanel",    GTK_STOCK_INFO,       _("RightPanel"),  0,     _("show/hide right panel"),  G_CALLBACK(&cb_view_menu_right_panel),  TRUE }
			, { "BottomPanel",   GTK_STOCK_INFO,       _("BottomPanel"), 0,     _("show/hide bottom panel"), G_CALLBACK(&cb_view_menu_bottom_panel), TRUE }
		};
		gtk_action_group_add_toggle_actions(action_group, entries, G_N_ELEMENTS(entries), app);
	}

	{
		static GtkRadioActionEntry entries[] = {
			  { "BottomPage1", 0, _("bottom page _1"), "<control>1", _("active bottom page 1"), 1 }
			, { "BottomPage2", 0, _("bottom page _2"), "<control>2", _("active bottom page 2"), 2 }
			, { "BottomPage3", 0, _("bottom page _3"), "<control>3", _("active bottom page 3"), 3 }
			, { "BottomPage4", 0, _("bottom page _4"), "<control>4", _("active bottom page 4"), 4 }
			, { "BottomPage5", 0, _("bottom page _5"), "<control>5", _("active bottom page 5"), 5 }
			, { "BottomPage6", 0, _("bottom page _6"), "<control>6", _("active bottom page 6"), 6 }
			, { "BottomPage7", 0, _("bottom page _7"), "<control>7", _("active bottom page 7"), 7 }
			, { "BottomPage8", 0, _("bottom page _8"), "<control>8", _("active bottom page 8"), 8 }
			, { "BottomPage9", 0, _("bottom page _9"), "<control>9", _("active bottom page 9"), 9 }
		};
		gtk_action_group_add_radio_actions( action_group
			, entries
			, G_N_ELEMENTS(entries)
			, 0
			, G_CALLBACK(&cb_view_menu_bottom_page_n)
			, app );
	}

	// ---------------------------------------------------
	// create UI
	{
		const gchar ui_info[] = 
			"<ui>"
				"<menubar name='MenuBar'>"
					"<menu action='FileMenu'>"
						"<menuitem action='New'/>"
						"<menuitem action='Open'/>"
						"<menuitem action='Save'/>"
						"<menuitem action='SaveAs'/>"
						"<menuitem action='Close'/>"
						"<separator/>"
						"<menuitem action='Quit'/>"
					"</menu>"

					"<menu action='EditMenu'>"
						"<!--"
						"<menuitem action='Undo'/>"
						"<menuitem action='Redo'/>"
						"<separator/>"
						"<menuitem action='Cut'/>"
						"<menuitem action='Copy'/>"
						"<separator/>"
						"-->"
						"<menuitem action='CmdLineGoto'/>"
						"<menuitem action='CmdLineFind'/>"
						"<menuitem action='CmdLineReplace'/>"
						"<separator/>"
						"<menuitem action='GoBack'/>"
						"<menuitem action='GoForward'/>"
						"<separator/>"
					"</menu>"

					"<menu action='ViewMenu'>"
						"<menuitem action='FullScreen'/>"
						"<separator/>"
						"<menuitem action='ActiveDocPage'/>"
						"<separator/>"
						"<menuitem action='LeftPanel'/>"
						"<menuitem action='RightPanel'/>"
						"<menuitem action='BottomPanel'/>"
						"<!--"
						"<separator/>"
						"<menuitem action='NextDocPage'/>"
						"-->"
						"<separator/>"
						"<menu action='BottomPages'>"
							"<menuitem action='BottomPage1'/>"
							"<menuitem action='BottomPage2'/>"
							"<menuitem action='BottomPage3'/>"
							"<menuitem action='BottomPage4'/>"
							"<menuitem action='BottomPage5'/>"
							"<menuitem action='BottomPage6'/>"
							"<menuitem action='BottomPage7'/>"
							"<menuitem action='BottomPage8'/>"
							"<menuitem action='BottomPage9'/>"
						"</menu>"
					"</menu>"

					"<menu action='ToolsMenu'>"
					"</menu>"

					"<menu action='PluginsMenu'>"
					"</menu>"

					"<menu action='HelpMenu'>"
						"<menuitem action='About'/>"
					"</menu>"

				"</menubar>"

				"<toolbar name='ToolBar'>"
					"<toolitem action='Open'/>"
					"<toolitem action='Quit'/>"
				"</toolbar>"
			"</ui>";

		app->main_window->ui_manager = gtk_ui_manager_new();

		if( !gtk_ui_manager_add_ui_from_string(app->main_window->ui_manager, ui_info, -1, &error) ) {
			g_message("create menu failed : %s", error->message);
			g_error_free(error);
		}
		gtk_ui_manager_insert_action_group(app->main_window->ui_manager, action_group, 0);
	}
}

// file menu

void cb_file_menu_new( GtkAction* action, Puss* app ) {
	puss_doc_new(app);
}

void cb_file_menu_open( GtkAction* action, Puss* app ) {
	puss_doc_open(app, 0, 0, 0);
}

void cb_file_menu_save( GtkAction* action, Puss* app ) {
	puss_doc_save_current(app, FALSE);
}

void cb_file_menu_save_as( GtkAction* action, Puss* app ) {
	puss_doc_save_current(app, TRUE);
}

void cb_file_menu_close( GtkAction* action, Puss* app ) {
	puss_doc_close_current(app);
}

void cb_file_menu_quit( GtkAction* action, Puss* app ) {
	gtk_widget_destroy(GTK_WIDGET(app->main_window->window));
}

void cb_edit_menu_goto( GtkAction* action, Puss* app ) {
	puss_mini_line_active(app, 0, 0, puss_mini_line_goto_get_callback());
}

void cb_edit_menu_find( GtkAction* action, Puss* app ) {
	g_message("find");
}

void cb_edit_menu_replace( GtkAction* action, Puss* app ) {
	g_message("replace");
}

void cb_edit_menu_go_back( GtkAction* action, Puss* app ) {
	g_message("go back");
}

void cb_edit_menu_go_forward( GtkAction* action, Puss* app ) {
	g_message("go forward");
}

void cb_view_menu_fullscreen( GtkAction* action, Puss* app ) {
	g_message("fullscreen");
}

void cb_view_menu_active_doc_page( GtkAction* action, Puss* app ) {
	g_message("active document page");
}

void cb_view_menu_left_panel( GtkAction* action, Puss* app ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(app->main_window->left_panel));
	else
		gtk_widget_hide(GTK_WIDGET(app->main_window->left_panel));
}

void cb_view_menu_right_panel( GtkAction* action, Puss* app ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(app->main_window->right_panel));
	else
		gtk_widget_hide(GTK_WIDGET(app->main_window->right_panel));
}

void cb_view_menu_bottom_panel( GtkAction* action, Puss* app ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(app->main_window->bottom_panel));
	else
		gtk_widget_hide(GTK_WIDGET(app->main_window->bottom_panel));
}

void cb_view_menu_bottom_page_n( GtkAction* action, GtkRadioAction* current, Puss* app ) {
	gint page_num = gtk_radio_action_get_current_value(current);
	//g_message("bottom page : %d", page_num);
	puss_active_panel_page(app->main_window->bottom_panel, page_num);
}

void cb_help_menu_about( GtkAction* action, Puss* app ) {
	puss_show_about_dialog(app->main_window->window);
}

