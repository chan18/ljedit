// Tips.h
// 

#ifndef PUSS_PLUGIN_INC_LANGUAGE_TIPS_H
#define PUSS_PLUGIN_INC_LANGUAGE_TIPS_H

#include "IPuss.h"

#include "cpp/guide.h"

#include <libintl.h>

#define TEXT_DOMAIN "language_tips"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct {
	Puss* app;

	CppGuide*		cpp_guide;

	GAsyncQueue*	parse_queue;
	GThread*		parse_thread;

	GtkBuilder*		builder;

	// outline window
	GtkWidget*		outline_panel;
	GtkTreeView*	outline_view;
	GtkTreeStore*	outline_store;
	CppFile*		outline_file;
	gint			outline_pos;

	// preview window
	GtkWidget*		preview_panel;
	GtkLabel*		preview_filename_label;
	GtkButton*		preview_number_button;
	GtkTextView*	preview_view;

	// tips window
	GtkWidget*		tips_include_window;
	GtkTreeView*	tips_include_view;
	GtkTreeModel*	tips_include_model;
	//StringSet*		tips_include_files;

	GtkWidget*		tips_list_window;
	GtkTreeView*	tips_list_view;
	GtkTreeModel*	tips_list_model;

	GtkWidget*		tips_decl_window;
	GtkTextView*	tips_decl_view;
	GtkTextBuffer*	tips_decl_buffer;

	// signal handlers
	gulong			page_added_handler_id;
	gulong			page_removed_handler_id;

	// update timer
	guint			update_timer;

	// regex
	GRegex*			re_include;
	GRegex*			re_include_tip;
	GRegex*			re_include_info;

} LanguageTips;

void parse_thread_init(LanguageTips* self);
void parse_thread_final(LanguageTips* self);

void ui_create(LanguageTips* self);
void ui_destroy(LanguageTips* self);

void outline_update(LanguageTips* self);

void preview_set(LanguageTips* self, const gchar* key, const gchar* key_text, CppFile* file, gint line);
void preview_update(LanguageTips* self);

#endif//PUSS_PLUGIN_INC_LANGUAGE_TIPS_H

