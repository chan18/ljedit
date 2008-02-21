// PluginEngine.cpp
//

#include "PluginEngine.h"

#include <gmodule.h>
#include <memory.h>

#include "IPuss.h"


struct Plugin {
	GModule*	module;
	Plugin*		plugin;

	Plugin*		prev;
	Plugin*		next;
};

void puss_plugin_engine_create(Puss* app) {
	if( !g_module_supported() )
		return;

	PluginEngine* self = (PluginEngine*)g_malloc(sizeof(PluginEngine));
	memset(self, 0, sizeof(PluginEngine));
}

void puss_plugin_engine_destroy(Puss* app) {
	if( !g_module_supported() )
		return;

	if( app && app->plugin_engine )
		g_free(app->plugin_engine);
}

void puss_plugin_engine_load(Puss* app) {
	if( !g_module_supported() )
		return;

}

