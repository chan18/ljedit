// IPuss.h
//

#ifndef PUSS_INC_IPUSS_H
#define PUSS_INC_IPUSS_H

#include <gtk/gtk.h>

struct MiniLineCallback {
	gpointer		 tag;

	gboolean 		(*cb_active)( gpointer tag );
	gboolean 		(*cb_key_press)( GdkEventKey* event, gpointer tag );
	void     		(*cb_changed)( gpointer tag );
};

typedef gboolean	(*OptionSetter)(GtkWindow* parent, GKeyFile* options, const gchar* group, const gchar* key, gpointer tag);
typedef void		(*OptionChanged)(const gchar* key, const gchar* group, const gchar* new_value, const gchar* current_value, gpointer tag);

struct Puss {
	// app
	const gchar*	(*get_module_path)();

	// UI
	GtkBuilder*		(*get_ui_builder)();

	// doc & view
	void			(*doc_set_url)( GtkTextBuffer* buffer, const gchar* url );
	GString*		(*doc_get_url)( GtkTextBuffer* buffer );

	void			(*doc_set_charset)( GtkTextBuffer* buffer, const gchar* charset );
	GString*		(*doc_get_charset)( GtkTextBuffer* buffer );

	GtkTextView*	(*doc_get_view_from_page)( GtkWidget* page );
	GtkTextBuffer*	(*doc_get_buffer_from_page)( GtkWidget* page );

	// doc manager
	GtkTextView*	(*doc_get_view_from_page_num)( gint page_num );
	GtkTextBuffer*	(*doc_get_buffer_from_page_num)( gint page_num );

	gint			(*doc_find_page_from_url)( const gchar* url );

	void			(*doc_new)();
	gboolean		(*doc_open)( const gchar* url, gint line, gint line_offset );
	gboolean		(*doc_locate)( gint page_num, gint line, gint line_offset, gboolean add_pos_locate );
	void			(*doc_save_current)( gboolean save_as );
	gboolean		(*doc_close_current)();
	void			(*doc_save_all)();
	gboolean		(*doc_close_all)();

	// mini line
	void			(*mini_line_active)( MiniLineCallback* cb );
	void			(*mini_line_deactive)();

	// utils
	void			(*send_focus_change)( GtkWidget* widget, gboolean in );
	void			(*active_panel_page)( GtkNotebook* panel, gint page_num );
	gboolean		(*load_file)(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, GError** err);

	// option manager
	gboolean		(*option_manager_option_reg)(const gchar* group, const gchar* key, const gchar* default_value, OptionSetter fun, gpointer tag);
	gboolean		(*option_manager_monitor_reg)(const gchar* group, const gchar* key, OptionChanged fun, gpointer tag);
};

#ifdef  __cplusplus
#	define __EXTERN_C extern "C"
#else
#	define __EXTERN_C
#endif

#ifdef G_OS_WIN32
#	define	PUSS_EXPORT		__EXTERN_C __declspec(dllexport)
#else
#	define	PUSS_EXPORT		__EXTERN_C
#endif

#define	SIGNAL_CALLBACK	PUSS_EXPORT


// utils functions
// 
inline GObject*			puss_get_ui_object(Puss* app, const gchar* id)	{ return gtk_builder_get_object(app->get_ui_builder(), id); }

inline GtkWindow*		puss_get_main_window(Puss* app)					{ return GTK_WINDOW(puss_get_ui_object(app, "main_window")); }
inline GtkUIManager*	puss_get_ui_manager(Puss* app)					{ return GTK_UI_MANAGER(puss_get_ui_object(app, "main_ui_manager")); }
inline GtkNotebook*		puss_get_doc_panel(Puss* app)					{ return GTK_NOTEBOOK(puss_get_ui_object(app, "doc_panel")); }
inline GtkNotebook*		puss_get_left_panel(Puss* app)					{ return GTK_NOTEBOOK(puss_get_ui_object(app, "left_panel")); }
inline GtkNotebook*		puss_get_right_panel(Puss* app)					{ return GTK_NOTEBOOK(puss_get_ui_object(app, "right_panel")); }
inline GtkNotebook*		puss_get_bottom_panel(Puss* app)				{ return GTK_NOTEBOOK(puss_get_ui_object(app, "bottom_panel")); }
inline GtkStatusbar*	puss_get_statusbar(Puss* app)					{ return GTK_STATUSBAR(puss_get_ui_object(app, "statusbar")); }

inline GtkLabel*		puss_get_mini_window_label(Puss* app)			{ return GTK_LABEL(puss_get_ui_object(app, "mini_window_label")); }
inline GtkEntry*		puss_get_mini_window_entry(Puss* app)			{ return GTK_ENTRY(puss_get_ui_object(app, "mini_window_entry")); }

#endif//PUSS_INC_IPUSS_H

