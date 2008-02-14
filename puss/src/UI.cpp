// UI.cpp

#include "UI.h"

#include <glib/gi18n.h>

#include "Puss.h"
#include "Menu.h"

void puss_create_ui(Puss* app) {
	UI* ui = &app->ui;
	ui->main_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    GtkVBox* vbox = GTK_VBOX(gtk_vbox_new(FALSE, 0));
	gtk_container_add(GTK_CONTAINER(ui->main_window), GTK_WIDGET(vbox));

    // create ui manager
    puss_create_ui_manager(app);
	GtkAccelGroup* accel_group = gtk_ui_manager_get_accel_group(ui->ui_manager);
	gtk_window_add_accel_group(ui->main_window, accel_group);

    // menubar
    GtkWidget* menubar = gtk_ui_manager_get_widget(ui->ui_manager, "/MenuBar");
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // toolbar
    GtkWidget* toolbar = gtk_ui_manager_get_widget(ui->ui_manager, "/ToolBar");
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	// body hpanel
	GtkHPaned* hpaned = GTK_HPANED(gtk_hpaned_new());
	gtk_container_set_border_width(GTK_CONTAINER(hpaned), 3);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hpaned),  TRUE, TRUE, 0);
	{
		// left
		ui->left_panel = GTK_NOTEBOOK(gtk_notebook_new());
		gtk_notebook_set_tab_pos(ui->left_panel, GTK_POS_BOTTOM);
		gtk_paned_pack1(GTK_PANED(hpaned), GTK_WIDGET(ui->left_panel), FALSE, FALSE);

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
				ui->doc_panel = GTK_NOTEBOOK(gtk_notebook_new());
				gtk_paned_pack1(GTK_PANED(right_hpaned), GTK_WIDGET(ui->doc_panel), TRUE, FALSE);

				// right panel
				ui->right_panel = GTK_NOTEBOOK(gtk_notebook_new());
				gtk_widget_set_size_request(GTK_WIDGET(ui->right_panel), 200, -1);
				gtk_notebook_set_tab_pos(ui->right_panel, GTK_POS_BOTTOM);
				gtk_paned_pack2(GTK_PANED(right_hpaned), GTK_WIDGET(ui->right_panel), FALSE, FALSE);
			}

			// bottom
			ui->bottom_panel = GTK_NOTEBOOK(gtk_notebook_new());
			gtk_widget_set_size_request(GTK_WIDGET(ui->bottom_panel), -1, 200);
			gtk_notebook_set_tab_pos(ui->bottom_panel, GTK_POS_BOTTOM);
			gtk_paned_pack2(GTK_PANED(right_vpaned), GTK_WIDGET(ui->bottom_panel), FALSE, FALSE);
		}

	}

    // statusbar
    ui->status_bar = GTK_STATUSBAR(gtk_statusbar_new());
    gtk_box_pack_end(GTK_BOX(vbox), GTK_WIDGET(ui->status_bar), FALSE, FALSE, 0);

	// cmd line
	//cmd_line_.create(*this);

	// show vbox & resize window
	gtk_widget_show_all(GTK_WIDGET(vbox));
	gtk_window_resize(ui->main_window, 1024, 768);
}

/* Cut and paste from gtkwindow.c */
void send_focus_change(GtkWidget *widget, gboolean in) {
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

void puss_active_panel_page(GtkNotebook* panel, gint page_num) {
	GtkWidget* w = gtk_notebook_get_nth_page(panel, page_num);
	if( w ) {
		gtk_notebook_set_current_page(panel, page_num);
		send_focus_change(w, TRUE);
	}
}

