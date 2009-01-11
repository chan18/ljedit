// DLLPluginEngine.c
//

#include "DLLPluginEngine.h"


typedef struct {
	GModule*	module;
	void*		handle;
} DLLPlugin;

gpointer dll_plugin_load(const gchar* plugin_id, GKeyFile* keyfile, Puss* app) {
	void* (*create_fun)(Puss* app);
	gchar* filename;
	gchar* filepath;
	DLLPlugin* dll_plugin;

	dll_plugin = g_try_new0(DLLPlugin, 1);
	if( !dll_plugin )
		return 0;

	filename = g_strconcat(plugin_id, ".so", 0);
	if( filename ) {
		filepath = g_build_filename(app->get_plugins_path(), filename, 0);
		if( filepath ) {
			dll_plugin->module = g_module_open(filepath, G_MODULE_BIND_LAZY);
			g_free(filepath);
		}
		g_free(filename);
	}

	if( !dll_plugin->module ) {
		g_printerr("ERROR  : load plugin(%s) failed!\n", filepath);
		g_printerr("REASON : %s\n", g_module_error());

	} else {
		g_module_symbol( dll_plugin->module
						, "puss_plugin_create"
						, (gpointer*)&create_fun );

		if( !create_fun ) {
			g_printerr("ERROR  : not find puss_plugin_create() in extend(%s)!\n", filepath);

		} else {
			dll_plugin->handle = (*create_fun)(app);
			return dll_plugin;
		}
	}

	if( dll_plugin->module )
		g_module_close(dll_plugin->module);

	g_free(dll_plugin);
	return 0;
}

void dll_plugin_unload(gpointer plugin, Puss* app) {
	DLLPlugin* dll_plugin = (DLLPlugin*)plugin;
	void (*destroy_fun)(void* handle);

	if( dll_plugin ) {
		if( dll_plugin->module ) {
			g_module_symbol(dll_plugin->module, "puss_plugin_destroy", (gpointer*)&destroy_fun);

			if( destroy_fun )
				(*destroy_fun)(dll_plugin->handle);

			g_module_close(dll_plugin->module);
		}

		g_free(dll_plugin);
	}
}

void dll_plugin_engine_regist(Puss* app) {
	app->plugin_engine_regist(DLL_PLUGIN_ENGINE_ID, dll_plugin_load, dll_plugin_unload, 0, app);
}
