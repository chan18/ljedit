// DLLPluginEngine.c
//

#include "DLLPluginEngine.h"


typedef struct {
	GModule*	module;
	void*		handle;
} DLLPlugin;

gpointer dll_plugin_load(Puss* app, const gchar* filepath) {
	void* (*create_fun)(Puss* app);
	DLLPlugin* dll_plugin = 0;

	dll_plugin = g_try_new0(DLLPlugin, 1);
	if( !dll_plugin )
		return 0;

	dll_plugin->module = g_module_open(filepath, G_MODULE_BIND_LAZY);
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
			return (*create_fun)(app);
		}
	}

	if( dll_plugin->module )
		g_module_close(dll_plugin->module);

	g_free(dll_plugin);
	return 0;
}

void dll_plugin_unload(Puss* app, gpointer plugin) {
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
	app->plugin_engine_regist("so", dll_plugin_load, dll_plugin_unload);
}