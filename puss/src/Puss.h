// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "IPuss.h"

typedef struct _MiniLine       MiniLine;
typedef struct _PosList        PosList;
typedef struct _Extend         Extend;
typedef struct _Utils          Utils;
typedef struct _OptionManager  OptionManager;

typedef struct {
	Puss			parent;

	GtkBuilder*		builder;
	gchar*			module_path;
	gchar*			locale_path;

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
} PussApp;

extern		PussApp*	puss_app;

gboolean	puss_create(const gchar* filepath);
void		puss_destroy();

void		puss_run();

#endif//PUSS_INC_PUSS_H

