// PluginManager.h
//

#ifndef PUSS_INC_PLUGINMANAGER_H
#define PUSS_INC_PLUGINMANAGER_H

#include "IPuss.h"

gboolean	puss_plugin_manager_create();
void		puss_plugin_manager_destroy();

void		puss_plugin_manager_show_config_dialog();

void		puss_plugin_manager_load_all();
void		puss_plugin_manager_unload_all();

gboolean	puss_plugin_manager_load_plugin(const gchar* plugin_id);
void		puss_plugin_manager_unload_plugin(const gchar* plugin_id);

void		puss_plugin_engine_regist( const gchar* key
						, PluginLoader loader
						, PluginUnloader unloader
						, PluginEngineDestroy destroy
						, gpointer tag );

#endif//PUSS_INC_PLUGINMANAGER_H

