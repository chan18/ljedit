// PluginManager.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "PluginManager.h"
#include "LJEditorImpl.h"

#ifdef G_OS_WIN32
    #define LJED_PLUGIN_EXT    ".dll"
    #define LJED_PLUGIN_EXT_SZ 4

#else// LINUX
    #define LJED_PLUGIN_EXT    ".so"
    #define LJED_PLUGIN_EXT_SZ 3

#endif


void find_all_plugin_file(std::vector<std::string>& out, const std::string& path) {
	std::string filename;

	Glib::Dir dir(path);
	Glib::Dir::iterator it = dir.begin();
	Glib::Dir::iterator end = dir.end();
	for( ; it!=end; ++it ) {
		filename = *it;
		if( filename.size() > LJED_PLUGIN_EXT_SZ
		    && filename.substr(filename.size() - LJED_PLUGIN_EXT_SZ)==LJED_PLUGIN_EXT )
		{
			out.push_back(path + '/' + filename);
        }
	}
}

DllPlugin::DllPlugin(const std::string& plugin_name)
	: dll_(plugin_name)
	, plugin_(0) {}

DllPlugin::~DllPlugin() {
	destroy();
}

bool DllPlugin::create(LJEditor& editor) {
	if( !dll_ )
		return false;

	void* symbol = 0;
	if( !dll_.get_symbol(PLUGIN_CREATE_FUN_NAME, symbol) || symbol==0 )
		return false;

	TPluginCreateFn createfn = (TPluginCreateFn)symbol;
	plugin_ = (*createfn)(editor);

	if( !plugin_->ljed_plugin_create(dll_.get_name()) )
		return false;

	return plugin_ != 0;
}

void DllPlugin::destroy() {
	if( !dll_ )
		return;

	if( plugin_==0 )
		return;

	plugin_->ljed_plugin_destroy();

	void* symbol = 0;
	if( !dll_.get_symbol(PLUGIN_DESTROY_FUN_NAME, symbol) || symbol==0 )
		return;

	TPluginDestroyFn destroyfn = (TPluginDestroyFn)symbol;
	(*destroyfn)(plugin_);

	plugin_ = 0;
}

PluginManager::PluginManager() {}

PluginManager::~PluginManager() {}

void PluginManager::load_plugins() {
    std::vector<std::string> plugin_files;
    {
        TPluginPaths::iterator it = paths_.begin();
        TPluginPaths::iterator end = paths_.end();
        for( ; it!=end; ++it )
            find_all_plugin_file(plugin_files, *it);
    }

    {
        std::vector<std::string>::iterator it = plugin_files.begin();
        std::vector<std::string>::iterator end = plugin_files.end();
        for( ; it!=end; ++it )
            add(*it);
    }
}

void PluginManager::unload_plugins() {
    TPlugins::iterator it = plugins_.begin();
    TPlugins::iterator end = plugins_.end();
	for( ; it!=end; ++it )
		delete it->second;
    plugins_.clear();
}

bool PluginManager::add(const std::string& plugin_filename) {
    DllPlugin* dllplugin = new DllPlugin(plugin_filename);
	if( dllplugin==0 )
		return false;

	if( !dllplugin->create(LJEditorImpl::self()) ) {
		delete dllplugin;
		return false;
	}

    plugins_[plugin_filename] = dllplugin;
    return true;
}

void PluginManager::remove(const std::string& plugin_filename) {
    TPlugins::iterator it = plugins_.find(plugin_filename);
    if( it != plugins_.end() ) {
		delete it->second;
		plugins_.erase(it);
    }
}

