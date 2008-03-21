// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "IPuss.h"

struct MiniLine;
struct PosList;
struct Extend;
struct Utils;
struct OptionManager;

struct PussApp {
	Puss			parent;

	GtkBuilder*		builder;
	gchar*			module_path;

	// get pointer from "builder"
	// 
	GtkWindow*		main_window;
	GtkUIManager*	ui_manager;
	GtkNotebook*	doc_panel;
	GtkNotebook*	left_panel;
	GtkNotebook*	right_panel;
	GtkNotebook*	bottom_panel;
	GtkStatusbar*	statusbar;

	MiniLine*		mini_line;
	PosList*		pos_list;
	Extend*			extends_list;
	Utils*			utils;
	OptionManager*	option_manager;
};

extern		PussApp*	puss_app;

gboolean	puss_create(const char* filepath);
void		puss_destroy();

void		puss_run();

#endif//PUSS_INC_PUSS_H

