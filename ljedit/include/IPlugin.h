// IPlugin.h
// 

#ifndef LJED_INC_IPLUGIN_H
#define LJED_INC_IPLUGIN_H

#include "LJEditor.h"

class IPlugin {
public:
    IPlugin(LJEditor& editor) : editor_(editor) {}

    bool ljed_plugin_create()  { return on_create(); }
    void ljed_plugin_destroy() { on_destroy(); }

    LJEditor& editor() { return editor_; }

    // destroy method
    // 
    virtual ~IPlugin() {}

    // plugin id
    // 
    virtual const char* get_plugin_name() = 0;

protected:
    // when new succeed and add to plugin list
    // 
    virtual bool on_create() = 0;

    // when remove from plugin list
    // 
    virtual void on_destroy() = 0;

private:
    LJEditor& editor_;
};

// dll export

#define PLUGIN_CREATE_FUN_NAME	"plugin_create"
#define PLUGIN_DESTROY_FUN_NAME	"plugin_destroy"

typedef IPlugin* (*TPluginCreateFn)(LJEditor& editor);
typedef void (*TPluginDestroyFn)(IPlugin* plugin);

#ifdef WIN32
    #define LJED_PLUGIN_DLL_EXPORT extern "C" __declspec( dllexport )
#else
    #define LJED_PLUGIN_DLL_EXPORT extern "C"
#endif

#endif//LJED_INC_IPLUGIN_H

