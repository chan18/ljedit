// IPuss.h
//

#ifndef PUSS_INC_IPUSS_H
#define PUSS_INC_IPUSS_H

#include <gtk/gtk.h>

struct Puss;

struct MainWindow {
	GtkWindow*		window;

	GtkUIManager*	ui_manager;

	GtkNotebook*	doc_panel;

	GtkNotebook*	left_panel;
	GtkNotebook*	right_panel;
	GtkNotebook*	bottom_panel;

	GtkStatusbar*	status_bar;
};

struct MiniLine {
	GtkLabel*		label;
	GtkEntry*		entry;
};

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

	GtkLabel*		(*doc_get_label_from_page_num)	( Puss* app, gint page_num );
	GtkTextView*	(*doc_get_view_from_page_num)	( Puss* app, gint page_num );
	GtkTextBuffer*	(*doc_get_buffer_from_page_num)	( Puss* app, gint page_num );

	gint			(*doc_find_page_from_url)		( Puss* app, const gchar* url );

	void			(*doc_new)						( Puss* app );
	gboolean		(*doc_open)						( Puss* app, const gchar* url, gint line, gint line_offset );
	gboolean		(*doc_locate)					( Puss* app, const gchar* url, gint line, gint line_offset );
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
	MainWindow*		main_window;
	MiniLine*		mini_line;

	gchar*			module_path;

	C_API*			api;
};

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

#endif//PUSS_INC_IPUSS_H

