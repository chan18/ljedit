// MainWindow.h
//

#ifndef LJED_INC_MAINWINDOW_H
#define LJED_INC_MAINWINDOW_H

#include <gtk/gtk.h>

//#include "DocManager.h"
//#include "CmdLine.h"


#define LJEDIT_TYPE_MAINWINDOW				(ljedit_main_window_get_type())
#define LJEDIT_MAINWINDOW(object)			(G_TYPE_CHECK_INSTANCE_CAST((object),	LJEDIT_TYPE_MAINWINDOW,	LJEditMainWindow))
#define LJEDIT_MAINWINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),		LJEDIT_TYPE_MAINWINDOW,	LJEditMainWindowClass))
#define LJEDIT_IS_MAINWINDOW(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object),	LJEDIT_TYPE_MAINWINDOW))
#define LJEDIT_IS_MAINWINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),		LJEDIT_TYPE_MAINWINDOW))
#define LJEDIT_MAINWINDOW_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object),	LJEDIT_TYPE_MAINWINDOW,	LJEditMainWindowClass))

typedef struct _LJEditMainWindowClass {
	GtkWindowClass parent_class;
} LJEditMainWindowClass;

typedef struct _LJEditMainWindow {
	GtkWindow            window;

	GtkUIManager*        ui_manager;

	//LJEditDocManager*    doc_manager;

	//LJEditCmdLine*        cmd_line;

	GtkNotebook*        left_panel;
	GtkNotebook*        right_panel;
	GtkNotebook*        botton_panel;

	GtkStatusbar*        status_bar;
} LJEditMainWindow;

GtkType		ljedit_main_window_get_type();
GtkWidget*	ljedit_main_window_new();
void		ljedit_main_window_init(LJEditMainWindow* w);

#endif//LJED_INC_MAINWINDOW_H

