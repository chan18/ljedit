// IPuss.h
//

#ifndef PUSS_INC_IPUSS_H
#define PUSS_INC_IPUSS_H

#include <gtk/gtk.h>

typedef struct _Puss  Puss;

typedef gpointer (*PluginLoader)(const gchar* plugin_id, GKeyFile* keyfile, gpointer tag);
typedef void     (*PluginUnloader)(gpointer plugin, gpointer tag);
typedef void     (*PluginEngineDestroy)(gpointer tag);

typedef struct _Option Option;

typedef gboolean	(*OptionSetter)(GtkWindow* parent, Option* option, gpointer tag);
typedef void		(*OptionChanged)(const Option* option, const gchar* old, gpointer tag);

// option manager
struct _Option {
	gchar*	group;
	gchar*	key;
	gchar*	default_value;
	gchar*	value;
};

// main
struct _Puss {
	// app
	const gchar*	(*get_module_path)();
	const gchar*	(*get_locale_path)();
	const gchar*	(*get_extends_path)();
	const gchar*	(*get_plugins_path)();

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
	gboolean		(*doc_open)( const gchar* url, gint line, gint line_offset, gboolean show_message_if_open_failed );
	gboolean		(*doc_locate)( gint page_num, gint line, gint line_offset, gboolean add_pos_locate );
	void			(*doc_save_current)( gboolean save_as );
	gboolean		(*doc_close_current)();
	void			(*doc_save_all)();
	gboolean		(*doc_close_all)();

	// utils
	void			(*send_focus_change)( GtkWidget* widget, gboolean in );
	void			(*active_panel_page)( GtkNotebook* panel, gint page_num );
	gboolean		(*load_file)(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset);
	gchar*			(*format_filename)(const gchar* filename);

	// option manager
	const Option*	(*option_reg)(const gchar* group, const gchar* key, const gchar* default_value);
	const Option*	(*option_find)(const gchar* group, const gchar* key);
	void			(*option_set)(const Option* option, const gchar* value);

	gpointer		(*option_monitor_reg)(const Option* option, OptionChanged fun, gpointer tag, GFreeFunc tag_free_fun);
	void			(*option_monitor_unreg)(gpointer handler);

	// extend manager
	gpointer		(*extend_query)(const gchar* ext_name, const gchar* interface_name);

	// plugin manager
	void			(*plugin_engine_regist)( const gchar* key
						, PluginLoader loader
						, PluginUnloader unloader
						, PluginEngineDestroy destroy
						, gpointer tag );
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

/*
	Puss Extend DLL struct
	
	// create extend
	PUSS_EXPORT	void* puss_extend_create(Puss* app);

	// init extend(after all extends created)
	PUSS_EXPORT void  puss_extend_init(void* self);

	// final extend(before all extends destroy)
	PUSS_EXPORT void  puss_extend_final(void* self);

	// destroy extend
	PUSS_EXPORT void  puss_extend_destroy(void* self);
*/

#define	SIGNAL_CALLBACK	PUSS_EXPORT


// utils functions
// 
/*
inline GObject*			puss_get_ui_object(Puss* app, const gchar* id)	{ return gtk_builder_get_object(app->get_ui_builder(), id); }

inline GtkWindow*		puss_get_main_window(Puss* app)					{ return GTK_WINDOW(puss_get_ui_object(app, "main_window")); }
inline GtkUIManager*	puss_get_ui_manager(Puss* app)					{ return GTK_UI_MANAGER(puss_get_ui_object(app, "main_ui_manager")); }
inline GtkNotebook*		puss_get_doc_panel(Puss* app)					{ return GTK_NOTEBOOK(puss_get_ui_object(app, "doc_panel")); }
inline GtkNotebook*		puss_get_left_panel(Puss* app)					{ return GTK_NOTEBOOK(puss_get_ui_object(app, "left_panel")); }
inline GtkNotebook*		puss_get_right_panel(Puss* app)					{ return GTK_NOTEBOOK(puss_get_ui_object(app, "right_panel")); }
inline GtkNotebook*		puss_get_bottom_panel(Puss* app)				{ return GTK_NOTEBOOK(puss_get_ui_object(app, "bottom_panel")); }
inline GtkStatusbar*	puss_get_statusbar(Puss* app)					{ return GTK_STATUSBAR(puss_get_ui_object(app, "statusbar")); }
*/

#define puss_get_ui_object(app, id)		gtk_builder_get_object((app)->get_ui_builder(), (id))
#define puss_get_main_window(app)		GTK_WINDOW(puss_get_ui_object((app), "main_window"))
#define puss_get_ui_manager(app)		GTK_UI_MANAGER(puss_get_ui_object((app), "main_ui_manager"))
#define puss_get_doc_panel(app)			GTK_NOTEBOOK(puss_get_ui_object((app), "doc_panel"))
#define puss_get_left_panel(app)		GTK_NOTEBOOK(puss_get_ui_object((app), "left_panel"))
#define puss_get_right_panel(app)		GTK_NOTEBOOK(puss_get_ui_object((app), "right_panel"))
#define puss_get_bottom_panel(app)		GTK_NOTEBOOK(puss_get_ui_object((app), "bottom_panel"))
#define puss_get_statusbar(app)			GTK_STATUSBAR(puss_get_ui_object((app), "statusbar"))

#endif//PUSS_INC_IPUSS_H

