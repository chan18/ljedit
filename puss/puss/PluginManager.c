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

#define PLUGIN_SUFFIX ".plugin"

struct _Plugin {
	gchar*			id;
	GKeyFile*		keyfile;
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

static void unload_plugin( Plugin* plugin ) {
	if( plugin ) {
		if( plugin->handle && plugin->engine && plugin->engine->unload )
			plugin->engine->unload(plugin->handle, plugin->engine->tag);

		g_free(plugin->id);
		g_key_file_free(plugin->keyfile);
		g_free(plugin);
	}
}

static gboolean load_plugin( const gchar* plugin_id, const gchar* filepath ) {
	gchar* engine_id;
	Plugin* plugin;
	GError* error;

	plugin = g_try_new0(Plugin, 1);
	if( !plugin ) {
		g_printerr( "new Plugin memory error!\n" );
		return FALSE;
	}

	plugin->id = g_strdup(plugin_id);
	if( plugin->id ) {
		plugin->keyfile = g_key_file_new();
		if( plugin->keyfile ) {
			error = 0;
			g_key_file_load_from_file(plugin->keyfile, filepath, G_KEY_FILE_NONE, &error);
			if( error ) {
				g_printerr( "load plugin error : load (%s).plugin failed(%s)!\n", plugin_id, error->message );
				g_error_free(error);

			} else {
				error = 0;
				engine_id = g_key_file_get_value(plugin->keyfile, "plugin", "engine", &error);
				if( error ) {
					g_printerr( "get engine-key error when load plugin[%s] - (%s)\n", plugin_id, error->message );
					g_error_free(error);
				}
				plugin->engine = (PluginEngine*)g_hash_table_lookup(puss_app->plugin_engines_map, engine_id?engine_id:DLL_PLUGIN_ENGINE_ID);
				g_free(engine_id);

				if( plugin->engine ) {
					plugin->handle = plugin->engine->load( plugin->id
						, plugin->keyfile
						, plugin->engine->tag );

					if( plugin->handle ) {
						plugin->next = puss_app->plugins_list;
						puss_app->plugins_list = plugin;
						return TRUE;
					}
				}
			}
		}
	}

	unload_plugin(plugin);
	return FALSE;
}

gboolean puss_plugin_manager_load_plugin(const gchar* plugin_id) {
	gchar* filename;
	gchar* filepath;
	gboolean res = FALSE;

	filename = g_strconcat(plugin_id, PLUGIN_SUFFIX, 0);
	if( filename ) {
		filepath = g_build_filename(puss_app->plugins_path, filename, 0);
		if( filepath )
			res = load_plugin( plugin_id, filepath );
	}

	g_free(filename);
	g_free(filepath);
	return res;
}

void puss_plugin_manager_load_all() {
	const gchar* filename;
	gchar* plugin_path;
	GDir* dir;
	gsize len;
	gchar* plugin_id;

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

			len = (gsize)strlen(filename);
			if( !g_str_has_suffix(filename, PLUGIN_SUFFIX) )
				continue;

			plugin_id = g_strndup(filename, len - strlen(PLUGIN_SUFFIX));
			puss_plugin_manager_load_plugin(plugin_id);
			g_free(plugin_id);
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

		unload_plugin(t);
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

