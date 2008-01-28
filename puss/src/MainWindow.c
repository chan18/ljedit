
// MainWindow.c

#include <glib/gi18n.h>
#include "MainWindow.h"

static GtkWindowClass* parent_class;

static void ljedit_main_window_class_init(LJEditMainWindowClass* klass) {
	parent_class = GTK_WINDOW_CLASS(gtk_type_class(gtk_window_get_type()));
}

GtkType ljedit_main_window_get_type() {
	static guint type = 0;
	if( !type ) {
		GtkTypeInfo info = { "LJEditMainWindow"
			, sizeof(LJEditMainWindow)
			, sizeof(LJEditMainWindowClass)
			, (GtkClassInitFunc)&ljedit_main_window_class_init
			, (GtkObjectInitFunc)&ljedit_main_window_init
			, 0
			, 0
		};

		type = gtk_type_unique(gtk_window_get_type(), &info);
	}

	return type;
}

GtkWidget* ljedit_main_window_new() {
	return GTK_WIDGET(gtk_type_new(ljedit_main_window_get_type()));
}

void ljedit_main_window_init(LJEditMainWindow* w) {
	w->ui_manager   = gtk_ui_manager_new();
	//gtk_ui_manager_get_widget(w->ui_manager, "/MenuBar");
	//w->doc_manager  = ljedit_doc_manager_new();
	//w->cmd_line     = ljedit_cmd_line_new();
	w->left_panel   = GTK_NOTEBOOK(gtk_notebook_new());
	w->right_panel  = GTK_NOTEBOOK(gtk_notebook_new());
	w->botton_panel = GTK_NOTEBOOK(gtk_notebook_new());
	w->status_bar   = GTK_STATUSBAR(gtk_statusbar_new());

	//GtkVBox* vbox = GTK_VBOX(gtk_vbox_new(FALSE, 0));
	//gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(w->left_panel), FALSE, FALSE, 0);

	//gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(vbox));
	//gtk_widget_show_all(GTK_WIDGET(vbox));
}

