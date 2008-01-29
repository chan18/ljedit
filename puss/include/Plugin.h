// Plugin.h
//

#ifndef PUSS_INC_PLUGIN_H
#define PUSS_INC_PLUGIN_H

#include <gtk/gtk.h>

class PussPluginEnviron
{
public:	// Puss Main UI
	GtkWindow*			main_window;

	GtkUIManager*		ui_manager;

	//PussDocManager*		doc_manager;
	//PussCmdLine*		cmd_line;

	GtkNotebook*		left_panel;
	GtkNotebook*		right_panel;
	GtkNotebook*		botton_panel;

	GtkStatusbar*		status_bar;
};

#define//PUSS_INC_PLUGIN_H

