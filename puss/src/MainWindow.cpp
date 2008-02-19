// MainWindow.cpp

#include "MainWindow.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "Puss.h"
#include "Menu.h"
#include "Utils.h"

void puss_main_window_create(Puss* app) {
	app->main_window = (MainWindow*)g_malloc(sizeof(MainWindow));
	memset(app->main_window, 0, sizeof(MainWindow));

	app->main_window->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    GtkVBox* vbox = GTK_VBOX(gtk_vbox_new(FALSE, 0));
	gtk_container_add(GTK_CONTAINER(app->main_window->window), GTK_WIDGET(vbox));

    // create ui manager
    puss_create_ui_manager(app);
	GtkAccelGroup* accel_group = gtk_ui_manager_get_accel_group(app->main_window->ui_manager);
	gtk_window_add_accel_group(app->main_window->window, accel_group);

    // menubar
    GtkWidget* menubar = gtk_ui_manager_get_widget(app->main_window->ui_manager, "/MenuBar");
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // toolbar
    GtkWidget* toolbar = gtk_ui_manager_get_widget(app->main_window->ui_manager, "/ToolBar");
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	// body hpanel
	GtkHPaned* hpaned = GTK_HPANED(gtk_hpaned_new());
	gtk_container_set_border_width(GTK_CONTAINER(hpaned), 3);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hpaned),  TRUE, TRUE, 0);
	{
		// left
		app->main_window->left_panel = GTK_NOTEBOOK(gtk_notebook_new());
		gtk_notebook_set_tab_pos(app->main_window->left_panel, GTK_POS_BOTTOM);
		gtk_paned_pack1(GTK_PANED(hpaned), GTK_WIDGET(app->main_window->left_panel), FALSE, FALSE);

		// right vpaned - for bottom panel
		GtkVPaned* right_vpaned = GTK_VPANED(gtk_vpaned_new());
		gtk_paned_pack2(GTK_PANED(hpaned), GTK_WIDGET(right_vpaned), TRUE, FALSE);
		{
			// right_hpaned
			GtkHPaned* right_hpaned = GTK_HPANED(gtk_hpaned_new());
			gtk_container_set_border_width(GTK_CONTAINER(right_hpaned), 3);
			gtk_paned_pack1(GTK_PANED(right_vpaned), GTK_WIDGET(right_hpaned), TRUE, FALSE);
			{
				// doc panel
				app->main_window->doc_panel = GTK_NOTEBOOK(gtk_notebook_new());
				gtk_paned_pack1(GTK_PANED(right_hpaned), GTK_WIDGET(app->main_window->doc_panel), TRUE, FALSE);

				// right panel
				app->main_window->right_panel = GTK_NOTEBOOK(gtk_notebook_new());
				gtk_widget_set_size_request(GTK_WIDGET(app->main_window->right_panel), 200, -1);
				gtk_notebook_set_tab_pos(app->main_window->right_panel, GTK_POS_BOTTOM);
				gtk_paned_pack2(GTK_PANED(right_hpaned), GTK_WIDGET(app->main_window->right_panel), FALSE, FALSE);
			}

			// bottom
			app->main_window->bottom_panel = GTK_NOTEBOOK(gtk_notebook_new());
			gtk_widget_set_size_request(GTK_WIDGET(app->main_window->bottom_panel), -1, 200);
			gtk_notebook_set_tab_pos(app->main_window->bottom_panel, GTK_POS_BOTTOM);
			gtk_paned_pack2(GTK_PANED(right_vpaned), GTK_WIDGET(app->main_window->bottom_panel), FALSE, FALSE);
		}

	}

    // statusbar
    app->main_window->status_bar = GTK_STATUSBAR(gtk_statusbar_new());
    gtk_box_pack_end(GTK_BOX(vbox), GTK_WIDGET(app->main_window->status_bar), FALSE, FALSE, 0);

	// show vbox & resize window
	gtk_widget_show_all(GTK_WIDGET(vbox));
	gtk_window_resize(app->main_window->window, 1024, 768);
}

void puss_main_window_destroy( Puss* app ) {
	if( app->main_window ) {
		g_free( app->main_window );
		app->main_window = 0;
	}
}

void puss_active_panel_page(GtkNotebook* panel, gint page_num) {
	GtkWidget* w = gtk_notebook_get_nth_page(panel, page_num);
	if( w ) {
		gtk_notebook_set_current_page(panel, page_num);
		puss_send_focus_change(w, TRUE);
	}
}

