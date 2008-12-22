// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "IPuss.h"

typedef struct _OptionManager  OptionManager;
typedef struct _Extend         Extend;
typedef struct _Plugin         Plugin;
typedef struct _PluginEngine   PluginEngine;
typedef struct _PosList        PosList;
typedef struct _Utils          Utils;

typedef struct _PussApp        PussApp;

struct _PussApp {
	Puss			parent;

	GtkBuilder*		builder;
	gchar*			module_path;
	gchar*			locale_path;
	gchar*			extends_path;
	gchar*			plugins_path;

	// get pointer from "builder"
	// 
	GtkWindow*		main_window;
	GtkUIManager*	ui_manager;
	GtkNotebook*	doc_panel;
	GtkNotebook*	left_panel;
	GtkNotebook*	right_panel;
	GtkNotebook*	bottom_panel;
	GtkStatusbar*	statusbar;

	Extend*			extends_list;
	GHashTable*		extends_map;
	Plugin*			plugins_list;
	GHashTable*		plugin_engines_map;
	OptionManager*	option_manager;
	PosList*		pos_list;
	Utils*			utils;
};

extern		PussApp*	puss_app;

gboolean	puss_create(const gchar* filepath);
void		puss_destroy();

void		puss_run();

#endif//PUSS_INC_PUSS_H

