// IPuss.h
//

#ifndef PUSS_INC_IPUSS_H
#define PUSS_INC_IPUSS_H

#include <gtk/gtk.h>

struct Puss;

struct MiniLineCallback {
	gpointer		 tag;

	gboolean 		(*cb_active)	( Puss* app, gpointer tag );
	gboolean 		(*cb_key_press)	( Puss* app, GdkEventKey* event, gpointer tag );
	void     		(*cb_changed)	( Puss* app, gpointer tag );
};

struct C_API {
	// app

	// main window

	// doc & view
	void			(*doc_set_url)					( GtkTextBuffer* buffer, const gchar* url );
	GString*		(*doc_get_url)					( GtkTextBuffer* buffer );

	void			(*doc_set_charset)				( GtkTextBuffer* buffer, const gchar* charset );
	GString*		(*doc_get_charset)				( GtkTextBuffer* buffer );

	GtkTextView*	(*doc_get_view_from_page)		( GtkWidget* page );
	GtkTextBuffer*	(*doc_get_buffer_from_page)		( GtkWidget* page );

	// doc manager

	GtkTextView*	(*doc_get_view_from_page_num)	( Puss* app, gint page_num );
	GtkTextBuffer*	(*doc_get_buffer_from_page_num)	( Puss* app, gint page_num );

	gint			(*doc_find_page_from_url)		( Puss* app, const gchar* url );

	void			(*doc_new)						( Puss* app );
	gboolean		(*doc_open)						( Puss* app, const gchar* url, gint line, gint line_offset );
	gboolean		(*doc_locate)					( Puss* app, gint page_num, gint line, gint line_offset, gboolean add_pos_locate );
	void			(*doc_save_current)				( Puss* app, gboolean save_as );
	gboolean		(*doc_close_current)			( Puss* app );
	void			(*doc_save_all)					( Puss* app );
	gboolean		(*doc_close_all)				( Puss* app );

	// mini line
	void			(*mini_line_active)				( Puss* app, MiniLineCallback* cb );
	void			(*mini_line_deactive)			( Puss* app );

	// utils
	void			(*send_focus_change)			( GtkWidget* widget, gboolean in );
	void			(*active_panel_page)			( GtkNotebook* panel, gint page_num );
};

struct Puss {
	C_API*			api;

	GtkBuilder*		builder;

	gchar*			module_path;
};

inline GtkWindow*		puss_get_main_window(Puss* app)			{ return GTK_WINDOW(gtk_builder_get_object(app->builder, "main_window")); }
inline GtkUIManager*	puss_get_ui_manager(Puss* app)			{ return GTK_UI_MANAGER(gtk_builder_get_object(app->builder, "main_ui_manager")); }
inline GtkNotebook*		puss_get_doc_panel(Puss* app)			{ return GTK_NOTEBOOK(gtk_builder_get_object(app->builder, "doc_panel")); }
inline GtkNotebook*		puss_get_left_panel(Puss* app)			{ return GTK_NOTEBOOK(gtk_builder_get_object(app->builder, "left_panel")); }
inline GtkNotebook*		puss_get_right_panel(Puss* app)			{ return GTK_NOTEBOOK(gtk_builder_get_object(app->builder, "right_panel")); }
inline GtkNotebook*		puss_get_bottom_panel(Puss* app)		{ return GTK_NOTEBOOK(gtk_builder_get_object(app->builder, "bottom_panel")); }
inline GtkStatusbar*	puss_get_statusbar(Puss* app)			{ return GTK_STATUSBAR(gtk_builder_get_object(app->builder, "statusbar")); }

inline GtkLabel*		puss_get_mini_window_label(Puss* app)	{ return GTK_LABEL(gtk_builder_get_object(app->builder, "mini_window_label")); }
inline GtkEntry*		puss_get_mini_window_entry(Puss* app)	{ return GTK_ENTRY(gtk_builder_get_object(app->builder, "mini_window_entry")); }

#ifdef  __cplusplus
#	define __EXTERN_C extern "C"
#else
#	define __EXTERN_C
#endif

#ifdef WIN32
#	define	PUSS_EXPORT		__EXTERN_C __declspec(dllexport)
#else
#	define	PUSS_EXPORT		__EXTERN_C
#endif

#define	SIGNAL_CALLBACK	PUSS_EXPORT

#endif//PUSS_INC_IPUSS_H

