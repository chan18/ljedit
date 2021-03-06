// Tips.h
// 

// TODO :
//	1 remove all LanguageTips* self, replace it with extern LanguageTips* g_tips;
//	2 replace libintl with Glib intl wrapped.

#ifndef PUSS_PLUGIN_INC_LANGUAGE_TIPS_H
#define PUSS_PLUGIN_INC_LANGUAGE_TIPS_H

#include "IPuss.h"

#include "cpp/guide.h"
#include "cpp/searcher.h"

#include <libintl.h>

#define TEXT_DOMAIN "language_tips"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct _ControlsPriv ControlsPriv;

typedef struct {
	Puss* app;

	CppGuide*		cpp_guide;

	GAsyncQueue*	parse_queue;
	GThread*		parse_thread;

	GtkBuilder*		builder;

	// icons
	GdkPixbuf**		icons;

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

	gpointer		preview_search_key;
	GAsyncQueue*	preview_search_queue;
	GThread*		preview_search_thread;
	GSequence*		preview_search_seq;
	gint			preview_last_index;
	CppFile*		preview_last_file;

	// tips window
	GtkWidget*		tips_include_window;
	GtkTreeView*	tips_include_view;
	GtkTreeModel*	tips_include_model;
	GList*			tips_include_files;

	GtkWidget*		tips_list_window;
	GtkTreeView*	tips_list_view;
	GtkTreeModel*	tips_list_model;
	GSequence*		tips_list_seq;

	GtkWidget*		tips_decl_window;
	GtkTextView*	tips_decl_view;
	GtkTextBuffer*	tips_decl_buffer;
	GSequence*		tips_decl_seq;

	// sourceview operator
	GSequence*		jump_to_seq;
	gint			jump_to_index;

	// controls implement
	ControlsPriv*	controls_priv;

	// update timer
	guint			update_timer;

	// regex
	GRegex*			re_include;
	GRegex*			re_include_tip;
	GRegex*			re_include_info;

	// option
	gpointer		option_font_change_handler;
	gpointer		option_style_change_handler;

	gpointer		option_path_change_handler;

} LanguageTips;

void parse_thread_init(LanguageTips* self);
void parse_thread_final(LanguageTips* self);
void parse_thread_push(LanguageTips* self, const gchar* filename, gboolean force_rebuild);

void ui_create(LanguageTips* self);
void ui_destroy(LanguageTips* self);

void controls_init(LanguageTips* self);
void controls_final(LanguageTips* self);

void outline_update(LanguageTips* self);

void preview_init(LanguageTips* self);
void preview_final(LanguageTips* self);
void preview_set(LanguageTips* self, gpointer spath, CppFile* file, gint line);
void preview_update(LanguageTips* self);

void tips_init(LanguageTips* self);
void tips_final(LanguageTips* self);

#if GTK_MAJOR_VERSION==2
	#define tips_include_is_visible(self)	GTK_WIDGET_VISIBLE(self->tips_include_window)
	#define tips_list_is_visible(self)		GTK_WIDGET_VISIBLE(self->tips_list_window)
	#define tips_decl_is_visible(self)		GTK_WIDGET_VISIBLE(self->tips_decl_window)
#else
	#define tips_include_is_visible(self)	gtk_widget_get_visible(self->tips_include_window)
	#define tips_list_is_visible(self)		gtk_widget_get_visible(self->tips_list_window)
	#define tips_decl_is_visible(self)		gtk_widget_get_visible(self->tips_decl_window)
#endif
#define tips_is_visible(self) (tips_include_is_visible(self) || tips_list_is_visible(self) || tips_decl_is_visible(self))

#define tips_include_tip_hide(self)		gtk_widget_hide(self->tips_include_window)
#define tips_list_tip_hide(self)		gtk_widget_hide(self->tips_list_window)
#define tips_decl_tip_hide(self)		gtk_widget_hide(self->tips_decl_window)
#define tips_hide_all(self ) { tips_include_tip_hide(self); tips_list_tip_hide(self); tips_decl_tip_hide(self); }

void    tips_include_tip_show(LanguageTips* self, gint x, gint y, GList* files);
void    tips_list_tip_show(LanguageTips* self, gint x, gint y, GSequence* seq);
void    tips_decl_tip_show(LanguageTips* self, gint x, gint y, GSequence* seq);

const gchar* tips_include_get_selected(LanguageTips* self);
CppElem*	 tips_list_get_selected(LanguageTips* self);

void tips_select_next(LanguageTips* self);
void tips_select_prev(LanguageTips* self);

gboolean tips_locate_sub(LanguageTips* self, gint x, gint y, const gchar* key);

// utils
gboolean open_and_locate_elem(LanguageTips* self, CppElem* elem);

#endif//PUSS_PLUGIN_INC_LANGUAGE_TIPS_H

