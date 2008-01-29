// MainWindow.cpp

#include "MainWindow.h"

// file menu
void cb_puss_file_menu_new( GtkAction* action, MainWindow* main_window );
void cb_puss_file_menu_open( GtkAction* action, MainWindow* main_window );
void cb_puss_file_menu_save( GtkAction* action, MainWindow* main_window );
void cb_puss_file_menu_save_as( GtkAction* action, MainWindow* main_window );
void cb_puss_file_menu_close( GtkAction* action, MainWindow* main_window );
void cb_puss_file_menu_quit( GtkAction* action, MainWindow* main_window );


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

void MainWindow::create_ui_manager() {
	GtkActionGroup* action_group = gtk_action_group_new("LJEditActions");

	static GtkActionEntry entries[] = {
		// main menu
		  { "FileMenu",    0, "_File" }
		, { "EditMenu",    0, "_Edit" }
		, { "ViewMenu",    0, "_View" }
		, { "ToolsMenu",   0, "_Tools" }
		, { "PluginsMenu", 0, "_Plugins" }
		, { "HelpMenu",    0, "_Help" }

		// file menu
		, { "New",    GTK_STOCK_NEW,     "_New",         "<control>N",        "create new file",    G_CALLBACK(&cb_puss_file_menu_new) }
		, { "Open",   GTK_STOCK_OPEN,    "_Open",        "<control>O",        "open file",          G_CALLBACK(&cb_puss_file_menu_open) }
		, { "Save",   GTK_STOCK_SAVE,    "_Save",        "<control>S",        "save file",          G_CALLBACK(&cb_puss_file_menu_save) }
		, { "SaveAs", GTK_STOCK_SAVE_AS, "Save _As ...", "<control><shift>S", "save file as...",    G_CALLBACK(&cb_puss_file_menu_save_as) }
		, { "Close",  GTK_STOCK_CLOSE,   "_Close",       "<control>W",        "close current file", G_CALLBACK(&cb_puss_file_menu_close) }
		, { "Quit",   GTK_STOCK_QUIT,    "_Quit",        "<control>Q",        "quit",               G_CALLBACK(&cb_puss_file_menu_quit) }

		// 
	};
	static guint n_entries = G_N_ELEMENTS(entries);

	gtk_action_group_add_actions(action_group, entries, n_entries, this);

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

void cb_puss_file_menu_new( GtkAction* action, MainWindow* main_window ) {
	g_message("new");
}

void cb_puss_file_menu_open( GtkAction* action, MainWindow* main_window ) {
	g_message("open");
}

void cb_puss_file_menu_save( GtkAction* action, MainWindow* main_window ) {
	g_message("save");
}

void cb_puss_file_menu_save_as( GtkAction* action, MainWindow* main_window ) {
	g_message("save as");
}

void cb_puss_file_menu_close( GtkAction* action, MainWindow* main_window ) {
	g_message("close");
}

void cb_puss_file_menu_quit( GtkAction* action, MainWindow* main_window ) {
	g_message("quit");
}
