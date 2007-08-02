// PluginManager.cpp
// 

#include "PluginManager.h"
#include "LJEditorImpl.h"

#ifdef WIN32
    #include <windows.h>

	#ifdef UNICODE
		inline HMODULE dlopen(const char* lib, int /*nouse*/) {
			assert( lib != 0 );
			std::string str(lib);
		    std::wstring wlib;
			wlib.resize(str.size());
			std::copy(str.begin(), str.end(), wlib.begin());
			return ::LoadLibrary(wlib.c_str());
		}
	#else
		#define dlopen(lib, nouse)   LoadLibrary(lib)
	#endif

    #define RTLD_LAZY 0

	#define dlsym(dll, name)  GetProcAddress(dll, name)
	#define dlclose(dll)      FreeLibrary(dll)

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
        plugin_destroy(it->second);
    plugins_.clear();
}

bool PluginManager::add(const std::string& plugin_filename) {
    DllPlugin dllplugin = { 0, 0 };
    if( !plugin_create(dllplugin, plugin_filename) )
        return false;
    
    plugins_[plugin_filename] = dllplugin;
    return true;
}

void PluginManager::remove(const std::string& plugin_filename) {
    TPlugins::iterator it = plugins_.find(plugin_filename);
    if( it != plugins_.end() ) {
        plugin_destroy(it->second);
        plugins_.erase(it);
    }
}

bool PluginManager::plugin_create(DllPlugin& dllplugin, const std::string& plugin_filename) {
    if( find(plugin_filename) != 0 )
        return false;;

    dllplugin.dll = 0;
    dllplugin.plugin = 0;

	dllplugin.dll = dlopen( plugin_filename.c_str(), RTLD_LAZY );
    if( dllplugin.dll != 0 ) {
		TPluginCreateFn createfn = (TPluginCreateFn)dlsym( dllplugin.dll, PLUGIN_CREATE_FUN_NAME );
        TPluginDestroyFn destroyfn = (TPluginDestroyFn)dlsym( dllplugin.dll, PLUGIN_DESTROY_FUN_NAME );
        if( createfn!=0 || destroyfn!=0 ) {
            dllplugin.plugin = (*createfn)(LJEditorImpl::self());
            if( dllplugin.plugin != 0 ) {
                if( dllplugin.plugin->ljed_plugin_create(plugin_filename) )
                    return true;
            }
        }
    }

    plugin_destroy(dllplugin);
    return false;
}

void PluginManager::plugin_destroy(DllPlugin& dllplugin) {
    if( dllplugin.dll != 0 ) {
        if( dllplugin.plugin != 0 ) {
			TPluginDestroyFn destroyfn = (TPluginDestroyFn)dlsym( dllplugin.dll, PLUGIN_DESTROY_FUN_NAME );
            if( destroyfn!=0 ) {
                dllplugin.plugin->ljed_plugin_destroy();
                destroyfn(dllplugin.plugin);
                dllplugin.plugin = 0;
            }
        }
		dlclose(dllplugin.dll);
        dllplugin.dll = 0;
    }
}

