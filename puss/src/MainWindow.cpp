// MainWindow.cpp

#include "MainWindow.h"

#include <glib/gi18n.h>

#include "AboutDialog.h"

/* Cut and paste from gtkwindow.c */
void send_focus_change(GtkWidget *widget, gboolean in)
{
	GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

	g_object_ref (widget);
   
	if (in)
		GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	else
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	fevent->focus_change.type = GDK_FOCUS_CHANGE;
	fevent->focus_change.window = GDK_WINDOW (g_object_ref(widget->window));
	fevent->focus_change.in = in;
  
	gtk_widget_event (widget, fevent);
  
	g_object_notify (G_OBJECT (widget), "has-focus");

	g_object_unref (widget);
	gdk_event_free (fevent);
}

MainWindow::MainWindow()
	: window(0)
	, ui_manager(0)
	, doc_manager(0)
	, left_panel(0)
	, right_panel(0)
	, bottom_panel(0)
	, status_bar(0)
{
}

MainWindow::~MainWindow()
{
}

void MainWindow::create(const std::string& path) {
	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    GtkVBox* vbox = GTK_VBOX(gtk_vbox_new(FALSE, 0));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));

    // create ui manager
    create_ui_manager();
	GtkAccelGroup* accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(window, accel_group);

    // menubar
    GtkWidget* menubar = gtk_ui_manager_get_widget(ui_manager, "/MenuBar");
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // toolbar
    GtkWidget* toolbar = gtk_ui_manager_get_widget(ui_manager, "/ToolBar");
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	// body hpanel
	GtkHPaned* hpaned = GTK_HPANED(gtk_hpaned_new());
	gtk_container_set_border_width(GTK_CONTAINER(hpaned), 3);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hpaned),  TRUE, TRUE, 0);
	{
		// left
		left_panel = GTK_NOTEBOOK(gtk_notebook_new());
		gtk_notebook_set_tab_pos(left_panel, GTK_POS_BOTTOM);
		gtk_paned_pack1(GTK_PANED(hpaned), GTK_WIDGET(left_panel), FALSE, FALSE);

		// right vpaned - for bottom panel
		GtkVPaned* right_vpaned = GTK_VPANED(gtk_vpaned_new());
		gtk_paned_pack2(GTK_PANED(hpaned), GTK_WIDGET(right_vpaned), TRUE, FALSE);
		{
			// right_hpaned
			GtkHPaned* right_hpaned = GTK_HPANED(gtk_hpaned_new());
			gtk_container_set_border_width(GTK_CONTAINER(right_hpaned), 3);
			gtk_paned_pack1(GTK_PANED(right_vpaned), GTK_WIDGET(right_hpaned), TRUE, FALSE);
			{
				// doc manager
				doc_manager = GTK_NOTEBOOK(gtk_notebook_new());
				gtk_paned_pack1(GTK_PANED(right_hpaned), GTK_WIDGET(doc_manager), TRUE, FALSE);
				//doc_manager_.create(path);

				// right panel
				right_panel = GTK_NOTEBOOK(gtk_notebook_new());
				gtk_widget_set_size_request(GTK_WIDGET(right_panel), 200, -1);
				gtk_notebook_set_tab_pos(right_panel, GTK_POS_BOTTOM);
				gtk_paned_pack2(GTK_PANED(right_hpaned), GTK_WIDGET(right_panel), FALSE, FALSE);
			}

			// bottom
			bottom_panel = GTK_NOTEBOOK(gtk_notebook_new());
			gtk_widget_set_size_request(GTK_WIDGET(bottom_panel), -1, 200);
			gtk_notebook_set_tab_pos(bottom_panel, GTK_POS_BOTTOM);
			gtk_paned_pack2(GTK_PANED(right_vpaned), GTK_WIDGET(bottom_panel), FALSE, FALSE);
		}

	}

    // statusbar
    status_bar = GTK_STATUSBAR(gtk_statusbar_new());
    gtk_box_pack_end(GTK_BOX(vbox), GTK_WIDGET(status_bar), FALSE, FALSE, 0);

	// cmd line
	//cmd_line_.create(*this);

	// show vbox & resize window
	gtk_widget_show_all(GTK_WIDGET(vbox));
	gtk_window_resize(window, 1024, 768);
}

void MainWindow::destroy() {
}

void MainWindow::active_and_focus_on_bottom_page(gint page_num) {
	GtkWidget* w = gtk_notebook_get_nth_page(bottom_panel, page_num);
	if( w ) {
		gtk_notebook_set_current_page(bottom_panel, page_num);
		send_focus_change(w, TRUE);
	}
}

// file menu
void cb_file_menu_new( GtkAction* action, MainWindow* main_window );
void cb_file_menu_open( GtkAction* action, MainWindow* main_window );
void cb_file_menu_save( GtkAction* action, MainWindow* main_window );
void cb_file_menu_save_as( GtkAction* action, MainWindow* main_window );
void cb_file_menu_close( GtkAction* action, MainWindow* main_window );
void cb_file_menu_quit( GtkAction* action, MainWindow* main_window );

// edit menu
void cb_edit_menu_goto( GtkAction* action, MainWindow* main_window );
void cb_edit_menu_find( GtkAction* action, MainWindow* main_window );
void cb_edit_menu_replace( GtkAction* action, MainWindow* main_window );
void cb_edit_menu_go_back( GtkAction* action, MainWindow* main_window );
void cb_edit_menu_go_forward( GtkAction* action, MainWindow* main_window );

// view menu
void cb_view_menu_fullscreen( GtkAction* action, MainWindow* main_window );

void cb_view_menu_active_doc_page( GtkAction* action, MainWindow* main_window );

void cb_view_menu_left_panel( GtkAction* action, MainWindow* main_window );
void cb_view_menu_right_panel( GtkAction* action, MainWindow* main_window );
void cb_view_menu_bottom_panel( GtkAction* action, MainWindow* main_window );

void cb_view_menu_bottom_page_n( GtkAction* action, GtkRadioAction* current, MainWindow* main_window );

// help menu
void cb_help_menu_about( GtkAction* action, MainWindow* main_window );


void MainWindow::create_ui_manager() {
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
		gtk_action_group_add_actions(action_group, entries, G_N_ELEMENTS(entries), this);
	}

	{
		static GtkToggleActionEntry entries[] = {
			  { "LeftPanel",     GTK_STOCK_INFO,       _("LeftPanel"),   0,     _("show/hide left panel"),   G_CALLBACK(&cb_view_menu_left_panel),   TRUE }
			, { "RightPanel",    GTK_STOCK_INFO,       _("RightPanel"),  0,     _("show/hide right panel"),  G_CALLBACK(&cb_view_menu_right_panel),  TRUE }
			, { "BottomPanel",   GTK_STOCK_INFO,       _("BottomPanel"), 0,     _("show/hide bottom panel"), G_CALLBACK(&cb_view_menu_bottom_panel), TRUE }
		};
		gtk_action_group_add_toggle_actions(action_group, entries, G_N_ELEMENTS(entries), this);
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
			, this );
	}

	// ---------------------------------------------------
	// create UI

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

	ui_manager = gtk_ui_manager_new();

	GError* error = 0;
	if( !gtk_ui_manager_add_ui_from_string(ui_manager, ui_info, -1, &error) ) {
		g_message("create menu failed : %s", error->message);
		g_error_free(error);
	}
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
}

// file menu

void cb_file_menu_new( GtkAction* action, MainWindow* main_window ) {
	g_message("new");
}

void cb_file_menu_open( GtkAction* action, MainWindow* main_window ) {
	g_message("open");
}

void cb_file_menu_save( GtkAction* action, MainWindow* main_window ) {
	g_message("save");
}

void cb_file_menu_save_as( GtkAction* action, MainWindow* main_window ) {
	g_message("save as");
}

void cb_file_menu_close( GtkAction* action, MainWindow* main_window ) {
	g_message("close");
}

void cb_file_menu_quit( GtkAction* action, MainWindow* main_window ) {
	gtk_widget_destroy(GTK_WIDGET(main_window->window));
}

void cb_edit_menu_goto( GtkAction* action, MainWindow* main_window ) {
	g_message("goto");
}

void cb_edit_menu_find( GtkAction* action, MainWindow* main_window ) {
	g_message("find");
}

void cb_edit_menu_replace( GtkAction* action, MainWindow* main_window ) {
	g_message("replace");
}

void cb_edit_menu_go_back( GtkAction* action, MainWindow* main_window ) {
	g_message("go back");
}

void cb_edit_menu_go_forward( GtkAction* action, MainWindow* main_window ) {
	g_message("go forward");
}

void cb_view_menu_fullscreen( GtkAction* action, MainWindow* main_window ) {
	g_message("fullscreen");
}

void cb_view_menu_active_doc_page( GtkAction* action, MainWindow* main_window ) {
	g_message("active document page");
}

void cb_view_menu_left_panel( GtkAction* action, MainWindow* main_window ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(main_window->left_panel));
	else
		gtk_widget_hide(GTK_WIDGET(main_window->left_panel));
}

void cb_view_menu_right_panel( GtkAction* action, MainWindow* main_window ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(main_window->right_panel));
	else
		gtk_widget_hide(GTK_WIDGET(main_window->right_panel));
}

void cb_view_menu_bottom_panel( GtkAction* action, MainWindow* main_window ) {
	gboolean active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if( active )
		gtk_widget_show(GTK_WIDGET(main_window->bottom_panel));
	else
		gtk_widget_hide(GTK_WIDGET(main_window->bottom_panel));
}

void cb_view_menu_bottom_page_n( GtkAction* action, GtkRadioAction* current, MainWindow* main_window ) {
	gint page_id = gtk_radio_action_get_current_value(current);
	//g_message("bottom page : %d", page_id);
	main_window->active_and_focus_on_bottom_page(page_id);
}

void cb_help_menu_about( GtkAction* action, MainWindow* main_window ) {
	puss_show_about_dialog(main_window->window);
}

