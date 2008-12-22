// PluginManager.c
//

#include "PluginManager.h"

#include <glib.h>
#include <gmodule.h>
#include <memory.h>
#include <string.h>

#include "Puss.h"
#include "Utils.h"
#include "DLLPluginEngine.h"

struct _Plugin {
	gchar*			filepath;
	PluginEngine*	engine;
	gpointer		handle;

	Plugin*			next;
};

struct _PluginEngine {
	gchar*				key;
	PluginLoader		load;
	PluginUnloader		unload;
	PluginEngineDestroy	destroy;
	gpointer			tag;
};

static void plugin_load(const gchar* plugin_path, const gchar* filename, PluginEngine* engine) {
	Plugin* plugin;

	plugin = g_try_new0(Plugin, 1);
	if( !plugin )
		return;

	plugin->filepath = g_build_filename(plugin_path, filename, NULL);
	if( !plugin->filepath ) {
		g_free(plugin);
		return;
	}

	plugin->engine = engine;
	plugin->handle = engine->load(plugin->filepath, plugin->engine->tag);

	plugin->next   = puss_app->plugins_list;
	puss_app->plugins_list = plugin;
}

static void plugin_unload(Plugin* plugin) {
	if( plugin ) {
		if( plugin->engine && plugin->engine->unload )
			plugin->engine->unload(plugin->handle, plugin->engine->tag);
		g_free(plugin);
	}
}

void puss_plugin_manager_load_all() {
	gchar* plugin_path;
	GDir* dir;
	PluginEngine* engine;
	const gchar* filename;
	size_t len;
	const gchar* ps;
	const gchar* pe;
	const gchar* prefix_str = "plugin_";
	size_t prefix_len = strlen(prefix_str);

	if( !g_module_supported() )
		return;

	plugin_path = puss_app->plugins_path;
	if( !plugin_path )
		return;

	dir = g_dir_open(plugin_path, 0, NULL);
	if( dir ) {
		for(;;) {
			filename = g_dir_read_name(dir);
			if( !filename )
				break;

			len = strlen(filename);
			if( !g_str_has_prefix(filename, prefix_str) )
				continue;

			ps = filename + prefix_len;
			pe = filename + len;
			for( ; pe > ps; --pe )
				if( *(pe-1)=='.' )
					break;

			engine = (PluginEngine*)g_hash_table_lookup(puss_app->plugin_engines_map, pe);
			if( !engine )
				continue;

			plugin_load(plugin_path, filename, engine);
		}

		g_dir_close(dir);
	}
}

void puss_plugin_manager_unload_all() {
	Plugin* p;
	Plugin* t;

	p = puss_app->plugins_list;
	puss_app->plugins_list = 0;

	while( p ) {
		t = p;
		p = p->next;

		plugin_unload(t);
	}
}

static void puss_plugin_engine_destroy(PluginEngine* engine) {
	if( engine ) {
		if( engine->destroy )
			engine->destroy(engine->tag);
		g_free(engine->key);
		g_free(engine);
	}
}

gboolean puss_plugin_manager_create() {
	puss_app->plugin_engines_map = g_hash_table_new_full(g_str_hash, g_str_equal, 0, puss_plugin_engine_destroy);
	if( !puss_app->plugin_engines_map )
		return FALSE;

	dll_plugin_engine_regist((Puss*)puss_app);

	return TRUE;
}

void puss_plugin_manager_destroy() {
	g_hash_table_destroy(puss_app->plugin_engines_map);
}


void puss_plugin_engine_regist( const gchar* key
		, PluginLoader loader
		, PluginUnloader unloader
		, PluginEngineDestroy destroy
		, gpointer tag )
{
	PluginEngine* engine = g_try_new0(PluginEngine, 1);
	if( engine ) {
		engine->key = g_strdup(key);
		engine->load = loader;
		engine->unload = unloader;
		engine->destroy = destroy;
		engine->tag = tag;

		g_hash_table_insert(puss_app->plugin_engines_map, engine->key, engine); 
	}
}

