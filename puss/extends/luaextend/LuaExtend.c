// LuaExtend.cpp
//

#include "LuaExtend.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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

//----------------------------------------------------------------
// IPuss wrappers

static int lua_wrapper_doc_new(lua_State* L) {
	g_self->app->doc_new();
	return 0;
}

static int lua_wrapper_doc_get_view_from_page_num(lua_State* L) {
	GtkTextView* view;
	gint pagenum = luaL_checkint(L, 1);

	view = g_self->app->doc_get_view_from_page_num(pagenum);
	if( !view )
		return 0;

	object_new(L, g_object_ref(view), TRUE);
	return 1;
}

static int lua_wrapper_doc_get_buffer_from_page_num(lua_State* L) {
	GtkTextBuffer* buf;
	gint pagenum = luaL_checkint(L, 1);

	buf = g_self->app->doc_get_buffer_from_page_num(pagenum);
	if( !buf )
		return 0;

	object_new(L, g_object_ref(buf), TRUE);
	return 1;
}


//----------------------------------------------------------------
// LuaExtend implements

typedef struct {
	gchar*		id;
	lua_State*	L;
} PussLuaPlugin;

static gpointer lua_plugin_load(const gchar* plugin_id, GKeyFile* keyfile, LuaExtend* self) {
	gchar* url = 0;
	PussLuaPlugin* plg = 0;
	gchar* sbuf;
	int err;

	sbuf = g_strdup_printf("%s.lua", plugin_id);
	url = g_build_filename(self->app->get_plugins_path(), sbuf, 0);
	g_free(sbuf);
	sbuf = 0;

	if( !url )
		goto finished;

	plg = g_new0(PussLuaPlugin, 1);
	plg->id = g_strdup(plugin_id);
	plg->L = lua_newthread(self->L);

	if( luaL_dofile(plg->L, url) ) {	// TODO : may be use dostring, url is utf-8 string
		goto finished;
	}

	lua_getglobal(plg->L, "puss_plugin_active");
	if( (err = lua_pcall(plg->L, 0, 0, 0)) != 0 ) {
		g_printerr("%s\n", lua_tostring(plg->L, -1));
	}


	g_print("load!!!\n");

finished:
	g_free(url);

	return plg;
}

static void lua_plugin_unload(gpointer plugin, LuaExtend* self) {
	PussLuaPlugin* plg = (PussLuaPlugin*)plugin;
	if( !plg )
		return;

	if( plg->L ) {
		lua_getglobal(plg->L, "puss_plugin_deactive");
		lua_pcall(plg->L, 0, 0, 0);
	}

	g_free(plg->id);
	g_free(plg);

	g_print("unloadl!!!\n");
}

static const struct luaL_reg puss_luaextend [] = {
	{"doc_new", lua_wrapper_doc_new},
	{"doc_get_view_from_page_num", lua_wrapper_doc_get_view_from_page_num},
	{"doc_get_buffer_from_page_num", lua_wrapper_doc_get_buffer_from_page_num},
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

