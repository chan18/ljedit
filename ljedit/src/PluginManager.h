// PluginManager.h
// 

#ifndef LJED_INC_PLUGIN_H
#define LJED_INC_PLUGIN_H

#include "IPlugin.h"

#include <string>
#include <map>
#include <set>

#ifdef WIN32
    #include <Windows.h>

    struct DllPlugin {
        HMODULE  dll;
        IPlugin* plugin;
    };
#else
    #include <dlfcn.h>

    struct DllPlugin {
        void*    dll;
        IPlugin* plugin;
    };
#endif

class PluginManager {
private:
    typedef std::map<std::string, DllPlugin>    TPlugins;
    typedef std::set<std::string>               TPluginPaths;

private:
    PluginManager();
    ~PluginManager();

    // nocopyable
    PluginManager(const PluginManager&);
    PluginManager& operator=(const PluginManager&);

public:
    static PluginManager& self() {
        static PluginManager me;
        return me;
    }

	void add_plugins_path(const std::string& path)
	    { paths_.insert(path); }

    TPlugins& plugins() { return plugins_; }

    void load_plugins();

    void unload_plugins();

    bool add(const std::string& plugin_filename);

    void remove(const std::string& plugin_filename);

    IPlugin* find(const std::string& plugin_filename) const {
        TPlugins::const_iterator it = plugins_.find(plugin_filename);
        return it!=plugins_.end() ? it->second.plugin : 0;
    }

private:
    bool plugin_create(DllPlugin& dllplugin, const std::string& plugin_filename);
    void plugin_destroy(DllPlugin& plugin);

private:
    TPlugins	    plugins_;
    TPluginPaths    paths_;
};

#endif//LJED_INC_IPLUGIN_H

