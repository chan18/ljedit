// IPuss.h
//

#ifndef PUSS_INC_IPUSS_H
#define PUSS_INC_IPUSS_H

#include <gtk/gtk.h>

typedef struct _Puss Puss;

typedef gboolean	(*FindLocation)(GtkTextBuffer* buf, gint* pline, gint* poffset, gpointer tag);

typedef gpointer	(*PluginLoader)(const gchar* plugin_id, GKeyFile* keyfile, gpointer tag);
typedef void		(*PluginUnloader)(gpointer plugin, gpointer tag);
typedef void		(*PluginEngineDestroy)(gpointer tag);

typedef struct _Option Option;

typedef gboolean	(*OptionSetter)(GtkWindow* parent, Option* option, gpointer tag);
typedef void		(*OptionChanged)(const Option* option, const gchar* old, gpointer tag);

typedef enum {
	  PUSS_PANEL_POS_HIDE	// TODO : Now It's equal LEFT
	, PUSS_PANEL_POS_LEFT
	, PUSS_PANEL_POS_RIGHT
	, PUSS_PANEL_POS_BOTTOM
} PanelPosition;

// option manager
struct _Option {
	gchar*	group;
	gchar*	key;
	gchar*	default_value;
	gchar*	value;
};

typedef GtkWidget* (*CreateSetupWidget)(gpointer tag);

// main
struct _Puss {
	// app
	const gchar*	(*get_module_path)();
	const gchar*	(*get_locale_path)();
	const gchar*	(*get_extends_path)();
	const gchar*	(*get_plugins_path)();

	// UI
	GtkBuilder*		(*get_ui_builder)();
	void			(*panel_append)(GtkWidget* panel, GtkWidget* tab_label, const gchar* id, PanelPosition default_pos);
	void			(*panel_remove)(GtkWidget* panel);
	gboolean		(*panel_get_pos)(GtkWidget* panel, GtkNotebook** parent, gint* page_num);

	// doc & view
	void			(*doc_set_url)( GtkTextBuffer* buffer, const gchar* url );
	GString*		(*doc_get_url)( GtkTextBuffer* buffer );

	void			(*doc_set_charset)( GtkTextBuffer* buffer, const gchar* charset );
	GString*		(*doc_get_charset)( GtkTextBuffer* buffer );

	void			(*doc_set_BOM)( GtkTextBuffer* buffer, gboolean BOM );
	gboolean		(*doc_get_BOM)( GtkTextBuffer* buffer );

	GtkTextView*	(*doc_get_view_from_page)( GtkWidget* page );
	GtkTextBuffer*	(*doc_get_buffer_from_page)( GtkWidget* page );

	// doc manager
	GtkTextView*	(*doc_get_view_from_page_num)( gint page_num );
	GtkTextBuffer*	(*doc_get_buffer_from_page_num)( gint page_num );

	gint			(*doc_find_page_from_url)( const gchar* url );

	gint			(*doc_new)();
	gboolean		(*doc_open)(const gchar* url, gint line, gint line_offset, gboolean show_message_if_open_failed );
	gboolean		(*doc_open_locate)(const gchar* url, FindLocation fun, gpointer tag, gboolean show_message_if_open_failed );
	gboolean		(*doc_locate)( gint page_num, gint line, gint line_offset, gboolean add_pos_locate );
	void			(*doc_save_current)( gboolean save_as );
	gboolean		(*doc_close_current)();
	void			(*doc_save_all)();
	gboolean		(*doc_close_all)();

	// utils
	void			(*send_focus_change)( GtkWidget* widget, gboolean in );
	void			(*active_panel_page)( GtkNotebook* panel, gint page_num );
	gboolean		(*save_file)(const gchar* filename, const gchar* text, gssize len, const gchar* charset, gboolean use_BOM);
	gboolean		(*load_file)(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, gboolean* use_BOM);
	gchar*			(*format_filename)(const gchar* filename);

	// option manager
	const Option*	(*option_reg)(const gchar* group, const gchar* key, const gchar* default_value);
	const Option*	(*option_find)(const gchar* group, const gchar* key);
	void			(*option_set)(const Option* option, const gchar* value);

	gpointer		(*option_monitor_reg)(const Option* option, OptionChanged fun, gpointer tag, GFreeFunc tag_free_fun);
	void			(*option_monitor_unreg)(gpointer handler);

	// option setup
	gboolean		(*option_setup_reg)(const gchar* id, const gchar* name, CreateSetupWidget creator, gpointer tag, GDestroyNotify tag_destroy);
	void			(*option_setup_unreg)(const gchar* id);


	// extend manager
	gpointer		(*extend_query)(const gchar* ext_name, const gchar* interface_name);

	// plugin manager
	void			(*plugin_engine_regist)( const gchar* key
						, PluginLoader loader
						, PluginUnloader unloader
						, PluginEngineDestroy destroy
						, gpointer tag );

	// search utils
	gboolean		(*find_and_locate_text)( GtkTextView* view
						, const gchar* text
						, gboolean is_forward
						, gboolean skip_current
						, gboolean mark_current
						, gboolean mark_all
						, gboolean is_continue
						, int search_flags );
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

