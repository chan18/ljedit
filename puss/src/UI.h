// UI.h
//

#ifndef PUSS_INC_UI_H
#define PUSS_INC_UI_H

#include <gtk/gtk.h>

struct UI {
	GtkWindow*			main_window;

	GtkUIManager*		ui_manager;

	GtkNotebook*		doc_panel;

	GtkNotebook*		left_panel;
	GtkNotebook*		right_panel;
	GtkNotebook*		bottom_panel;

	GtkStatusbar*		status_bar;

	GtkWindow*			cmd_line_window;
};

struct Puss;

void puss_create_ui(Puss* app);

void puss_active_panel_page(GtkNotebook* panel, gint page_num);

#endif//PUSS_INC_UI_H

