// MainWindow.h
//

#ifndef PUSS_INC_MAINWINDOW_H
#define PUSS_INC_MAINWINDOW_H

#include <gtk/gtk.h>
#include <string>

class MainWindow {
public:
	GtkWindow*			window;

	GtkUIManager*		ui_manager;

	GtkNotebook*		doc_manager;

	GtkNotebook*		left_panel;
	GtkNotebook*		right_panel;
	GtkNotebook*		bottom_panel;

	GtkStatusbar*		status_bar;

	//PussCmdLine		cmd_line;

public:
	MainWindow();
	~MainWindow();

    void create(const std::string& path);
    void destroy();

	void active_and_focus_on_bottom_page(gint page_num);

private:
    void create_ui_manager();
};

#endif//LJED_INC_MAINWINDOW_H

