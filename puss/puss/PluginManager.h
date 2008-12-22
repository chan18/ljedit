// PluginManager.h
//

#ifndef PUSS_INC_PLUGINMANAGER_H
#define PUSS_INC_PLUGINMANAGER_H

#include "IPuss.h"

gboolean	puss_plugin_manager_create();
void		puss_plugin_manager_destroy();

void		puss_plugin_engine_regist( const gchar* key
						, PluginEngineLoader* loader
						, PluginEngineUnloader* unloader );

#endif//PUSS_INC_PLUGINMANAGER_H

