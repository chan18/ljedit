// LuaExtend.cpp
//

#include "LuaExtend.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// copy from lgob

/**
 * An object representation. Holds a pointer and a flag that marks if the object
 * need special care on gc. 
 */
typedef struct
{
	void* pointer;
	char need_unref;
} Object;

struct _LuaExtend {
	Puss*			app;

	lua_State*		L;
};

static LuaExtend* g_self = 0;

static void register_class(lua_State* L, const char* name, const char* base, const luaL_Reg* reg)
{
	lua_pushstring(L, name);
	lua_newtable(L);
	luaL_register(L, NULL, reg);

	if(base)
	{
		lua_newtable(L);
		lua_pushliteral(L, "__index");
		lua_pushstring(L, base);
		lua_rawget(L, -6);
		lua_rawset(L, -3);
		lua_setmetatable(L, -2);
	}
	
	lua_rawset(L, -3); 
}

static void object_new(lua_State* L, gpointer ptr, gboolean constructor)
{
	lua_pushliteral(L, "lgobObjectNew");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, ptr);
	lua_pushboolean(L, constructor); 
	lua_call(L, 2, 1);
}

static gpointer check_lua_to_object(lua_State* L, int idx, GType is_a_type) {
	if( lua_isuserdata(L, idx) ) {
		Object* o = lua_touserdata(L, idx);
		if( o && o->pointer && g_type_is_a(G_OBJECT_TYPE(o->pointer), is_a_type) )
			return o->pointer;
	}

	luaL_argerror(L, idx, "arg not gobject");
	return 0; 
}

//----------------------------------------------------------------
// IPuss wrappers

static int lua_wrapper_get_module_path(lua_State* L) {
	lua_pushstring(L, g_self->app->get_module_path());
	return 1;
}

static int lua_wrapper_get_locale_path(lua_State* L) {
	lua_pushstring(L, g_self->app->get_locale_path());
	return 1;
}

static int lua_wrapper_get_extends_path(lua_State* L) {
	lua_pushstring(L, g_self->app->get_extends_path());
	return 1;
}

static int lua_wrapper_get_plugins_path(lua_State* L) {
	lua_pushstring(L, g_self->app->get_plugins_path());
	return 1;
}

static int lua_wrapper_get_ui_builder(lua_State* L) {
	object_new(L, g_self->app->get_ui_builder(), FALSE);
	return 1;
}

static int lua_wrapper_panel_append(lua_State* L) {
	GtkWidget* panel;
	GtkWidget* tab_label;
	const gchar* id;
	PanelPosition default_pos;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==4, 4, "invalid arg num");
	panel = (GtkWidget*)check_lua_to_object(L, 1, GTK_TYPE_WIDGET);
	tab_label = (GtkWidget*)check_lua_to_object(L, 2, GTK_TYPE_WIDGET);
	id = luaL_checkstring(L, 3);
	default_pos = luaL_checkinteger(L, 4);

	g_self->app->panel_append(panel, tab_label, id, default_pos);
	return 0;
}

static int lua_wrapper_panel_remove(lua_State* L) {
	GtkWidget* panel;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	panel = (GtkWidget*)check_lua_to_object(L, 1, GTK_TYPE_WIDGET);
	g_self->app->panel_remove(panel);
	return 0;
}

static int lua_wrapper_panel_get_pos(lua_State* L) {
	GtkNotebook* parent;
	gint page_num;
	GtkWidget* panel;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	panel = (GtkWidget*)check_lua_to_object(L, 1, GTK_TYPE_WIDGET);
	if( g_self->app->panel_get_pos(panel, &parent, &page_num) ) {
		object_new(L, parent, FALSE);
		lua_pushinteger(L, page_num);
		return 2;
	}
	return 0;
}

static int lua_wrapper_doc_set_url(lua_State* L) {
	GtkTextBuffer* buffer;
	const gchar* url;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==2, 2, "invalid arg num");
	buffer = (GtkTextBuffer*)check_lua_to_object(L, 1, GTK_TYPE_TEXT_BUFFER);
	url = luaL_checkstring(L, 2);

	g_self->app->doc_set_url(buffer, url);
	return 0;
}

static int lua_wrapper_doc_get_url(lua_State* L) {
	GtkTextBuffer* buffer;
	GString* url;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	buffer = (GtkTextBuffer*)check_lua_to_object(L, 1, GTK_TYPE_TEXT_BUFFER);

	url = g_self->app->doc_get_url(buffer);
	if( url ) {
		lua_pushlstring(L, url->str, url->len);
		return 1;
	}
	return 0;
}

static int lua_wrapper_doc_set_charset(lua_State* L) {
	GtkTextBuffer* buffer;
	const gchar* charset;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==2, 2, "invalid arg num");
	buffer = (GtkTextBuffer*)check_lua_to_object(L, 1, GTK_TYPE_TEXT_BUFFER);
	charset = luaL_checkstring(L, 2);

	g_self->app->doc_set_charset(buffer, charset);
	return 0;
}

static int lua_wrapper_doc_get_charset(lua_State* L) {
	GtkTextBuffer* buffer;
	GString* charset;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	buffer = (GtkTextBuffer*)check_lua_to_object(L, 1, GTK_TYPE_TEXT_BUFFER);

	charset = g_self->app->doc_get_charset(buffer);
	if( charset ) {
		lua_pushlstring(L, charset->str, charset->len);
		return 1;
	}
	return 0;
}

static int lua_wrapper_doc_get_view_from_page(lua_State* L) {
	GtkTextView* view;
	GtkWidget* page;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	page = (GtkWidget*)check_lua_to_object(L, 1, GTK_TYPE_WIDGET);
	view = g_self->app->doc_get_view_from_page(page);
	if( view ) {
		object_new(L, view, FALSE);
		return 1;
	}
	return 0;
}

static int lua_wrapper_doc_get_buffer_from_page(lua_State* L) {
	GtkTextBuffer* buffer;
	GtkWidget* page;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	page = (GtkWidget*)check_lua_to_object(L, 1, GTK_TYPE_WIDGET);
	buffer = g_self->app->doc_get_buffer_from_page(page);
	if( buffer ) {
		object_new(L, buffer, FALSE);
		return 1;
	}
	return 0;
}

static int lua_wrapper_doc_get_view_from_page_num(lua_State* L) {
	GtkTextView* view;
	gint page_num;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	page_num = luaL_checkinteger(L, 1);

	view = g_self->app->doc_get_view_from_page_num(page_num);
	if( view ) {
		object_new(L, view, FALSE);
		return 1;
	}
	return 0;
}

static int lua_wrapper_doc_get_buffer_from_page_num(lua_State* L) {
	GtkTextBuffer* buffer;
	gint page_num;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	page_num = luaL_checkinteger(L, 1);

	buffer = g_self->app->doc_get_buffer_from_page_num(page_num);
	if( buffer ) {
		object_new(L, buffer, FALSE);
		return 1;
	}
	return 0;
}

static int lua_wrapper_doc_find_page_from_url(lua_State* L) {
	const gchar* url;
	gint page_num = -1;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	url = luaL_checkstring(L, 1);
	if( url )
		page_num = g_self->app->doc_find_page_from_url(url);
	lua_pushinteger(L, page_num);
	return 1;
}

static int lua_wrapper_doc_new(lua_State* L) {
	g_self->app->doc_new();
	return 0;
}

static int lua_wrapper_doc_open(lua_State* L) {
	gboolean retval;
	const gchar* url;
	gint line;
	gint line_offset;
	gboolean show_message_if_open_failed;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==4, 4, "invalid arg num");
	url = luaL_checkstring(L, 1);
	line = luaL_checkinteger(L, 2);
	line_offset = luaL_checkinteger(L, 3);
	show_message_if_open_failed = lua_toboolean(L, 4);

	retval = g_self->app->doc_open(url, line, line_offset, show_message_if_open_failed);
	lua_pushboolean(L, retval);
	return 1;
}

static int lua_wrapper_doc_locate(lua_State* L) {
	gboolean retval;
	gint page_num;
	gint line;
	gint line_offset;
	gboolean add_pos_locate;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==4, 4, "invalid arg num");
	page_num = luaL_checkinteger(L, 1);
	line = luaL_checkinteger(L, 2);
	line_offset = luaL_checkinteger(L, 3);
	add_pos_locate = lua_toboolean(L, 4);

	retval = g_self->app->doc_locate(page_num, line, line_offset, add_pos_locate);
	lua_pushboolean(L, retval);
	return 1;
}

static int lua_wrapper_doc_save_current(lua_State* L) {
	gboolean save_as;
	int n = lua_gettop(L);		/* number of arguments */

	luaL_argcheck(L, n==1, 1, "invalid arg num");
	save_as = lua_toboolean(L, 1);

	g_self->app->doc_save_current(save_as);
	return 0;
}

static int lua_wrapper_doc_close_current(lua_State* L) {
	gboolean retval = g_self->app->doc_close_current();
	lua_pushboolean(L, retval);
	return 1;
}

static int lua_wrapper_doc_save_all(lua_State* L) {
	g_self->app->doc_save_all();
	return 0;
}

static int lua_wrapper_doc_close_all(lua_State* L) {
	gboolean retval = g_self->app->doc_close_all();
	lua_pushboolean(L, retval);
	return 1;
}

/*
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
*/


//----------------------------------------------------------------
// LuaExtend implements

// TODO : not use newthread
// You can't call lua_close() on a thread; only on the main state.
// 

static gpointer lua_plugin_load(const gchar* plugin_id, GKeyFile* keyfile, LuaExtend* self) {
	const char* code =
		"local id = '%s'\n"
		"local plg = dofile(puss.get_plugins_path() .. '/' .. id .. '.lua')\n"
		"plg.active()\n"
		"puss.plugins[id] = plg\n"
		;
	gchar* sbuf = g_strdup_printf(code, plugin_id);
	lua_State* L = self->L;
	int err = luaL_dostring(L, sbuf);
	g_free(sbuf);
	if( err ) {
		g_warning("lua_plugin_load(%s): %s\n", plugin_id, lua_tostring(self->L, -1));
		return 0;
	}

	return g_strdup(plugin_id);
}

static void lua_plugin_unload(gpointer plugin, LuaExtend* self) {
	if( plugin ) {
		gchar* plugin_id = (gchar*)plugin;
		const char* code =
			"local id = '%s'\n"
			"local plg = puss.plugins[id]\n"
			"plg.deactive()\n"
			;
		gchar* sbuf = g_strdup_printf(code, plugin_id);
		lua_State* L = self->L;
		int err = luaL_dostring(L, sbuf);
		g_free(sbuf);
		if( err )
			g_warning("lua_plugin_unload(%s): %s\n", plugin_id, lua_tostring(self->L, -1));
		g_free(plugin_id);
	}

	// g_print("unloadl!!!\n");
}

static const struct luaL_reg puss_luaextend [] = {
	{"get_module_path", lua_wrapper_get_module_path},
	{"get_locale_path", lua_wrapper_get_locale_path},
	{"get_extends_path", lua_wrapper_get_extends_path},
	{"get_plugins_path", lua_wrapper_get_plugins_path},
	{"get_ui_builder", lua_wrapper_get_ui_builder},
	{"panel_append", lua_wrapper_panel_append},
	{"panel_remove", lua_wrapper_panel_remove},
	{"panel_get_pos", lua_wrapper_panel_get_pos},
	{"doc_set_url", lua_wrapper_doc_set_url},
	{"doc_get_url", lua_wrapper_doc_get_url},
	{"doc_set_charset", lua_wrapper_doc_set_charset},
	{"doc_get_charset", lua_wrapper_doc_get_charset},
	{"doc_get_view_from_page", lua_wrapper_doc_get_view_from_page},
	{"doc_get_buffer_from_page", lua_wrapper_doc_get_buffer_from_page},
	{"doc_get_view_from_page_num", lua_wrapper_doc_get_view_from_page_num},
	{"doc_get_buffer_from_page_num", lua_wrapper_doc_get_buffer_from_page_num},
	{"doc_find_page_from_url", lua_wrapper_doc_find_page_from_url},
	{"doc_new", lua_wrapper_doc_new},
	{"doc_open", lua_wrapper_doc_open},
	{"doc_locate", lua_wrapper_doc_locate},
	{"doc_save_current", lua_wrapper_doc_save_current},
	{"doc_close_current", lua_wrapper_doc_close_current},
	{"doc_save_all", lua_wrapper_doc_save_all},
	{"doc_close_all", lua_wrapper_doc_close_all},
	{NULL, NULL}
};

static const struct luaL_reg _global[] = {
	{NULL, NULL}
};

static const struct luaL_reg puss_text_view [] = {
	{NULL, NULL}
};

static void init_lua_puss(lua_State* L) {
	luaL_loadstring(L, "require('lgob.gtksourceview')"); lua_call(L, 0, 0);
	lua_getfield(L, LUA_GLOBALSINDEX, "gtk");
	register_class(L, "PussTextView", "SourceView", puss_text_view);

	luaL_register(L, "puss", puss_luaextend);
	luaL_register(L, NULL, _global);
	luaL_dostring(L, "puss.plugins = {}");
}

LuaExtend* puss_lua_extend_create(Puss* app) {
	gchar* path;
	lua_State* L = lua_open();
	if( L==0 )
		return 0;

	g_self = g_try_new0(LuaExtend, 1);
	if( !g_self )
		return 0;

	g_self->app = app;
	g_self->L = L;

	luaL_openlibs(L);

	// set package.path & package.cpath
	// 
	// package.path : 
	// ./?.lua
	// ./?/init.lua
	// $(extern-path)/?.lua
	// $(extern-path)/?/init.lua
	// $(plugin-path)/?.lua
	// $(plugin-path)/?/init.lua
	// 
	// package.cpath :
	// ./?.so
	// $(extern-path)/lua/?.so
	// 
	{
		lua_getglobal(L, "package");

		path = g_strdup_printf( "./?.lua;"
					"./?/init.lua;"
					"%s/?.lua;"
					"%s/?/init.lua;"
					"%s/?.lua;"
					"%s/?/init.lua"
					, app->get_extends_path()
					, app->get_extends_path()
					, app->get_plugins_path()
					, app->get_plugins_path() );
		lua_pushstring(L, path);
		lua_setfield(L, -2, "path");
		g_free(path);

#ifdef G_OS_WIN32
		path = g_strdup_printf( "./?.dll;%s/lua/?.dll", app->get_extends_path());
#else
		path = g_strdup_printf( "./?.so;%s/lua/?.so", app->get_extends_path());
#endif
		lua_pushstring(L, path);
		lua_setfield(L, -2, "cpath");
		g_free(path);
	}

	init_lua_puss(L);

	app->plugin_engine_regist( "lua"
		, (PluginLoader)lua_plugin_load
		, (PluginUnloader)lua_plugin_unload
		, 0
		, g_self );

	return g_self;
}

void puss_lua_extend_destroy(LuaExtend* self) {
	g_self = 0;
	if( self ) {
		// TODO : destroy engine

		lua_close(self->L);
		g_free(self);
	}
}

