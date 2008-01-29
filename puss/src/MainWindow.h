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

private:
    void create_ui_manager();
	
};

#endif//LJED_INC_MAINWINDOW_H

