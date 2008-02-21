// PluginEngine.h
//

#ifndef PUSS_INC_PLUGINENGINE_H
#define PUSS_INC_PLUGINENGINE_H

struct Puss;

void	puss_plugin_engine_create(Puss* app);
void	puss_plugin_engine_destroy(Puss* app);

void	puss_plugin_engine_load(Puss* app);

#endif//PUSS_INC_PLUGINENGINE_H

