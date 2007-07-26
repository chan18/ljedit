// LJCSPlugin.cpp
// 

#include "IPlugin.h"
#include "LJCSPluginImpl.h"

class LJCSPlugin : public IPlugin {
public:
    static IPlugin*		create_plugin(LJEditor& editor);

    virtual const char* get_plugin_name();

private:
    LJCSPlugin(LJEditor& editor, void* impl);
    virtual ~LJCSPlugin();

    virtual bool on_create();
    virtual void on_destroy();

private:
    void* impl_;
};

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return LJCSPlugin::create_plugin(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}


IPlugin* LJCSPlugin::create_plugin(LJEditor& editor) {
    LJCSPluginImpl* impl = new LJCSPluginImpl(editor);
    IPlugin* plugin = new LJCSPlugin(editor, impl);
    if( impl==0 || plugin==0 ) {
        delete impl;
        delete plugin;
        plugin = 0;
    }

    return plugin;
}

const char* LJCSPlugin::get_plugin_name() {
    return "__LJCS_PLUGIN__";
}

LJCSPlugin::LJCSPlugin(LJEditor& editor, void* impl)
    : IPlugin(editor)
    , impl_(impl) {
}

LJCSPlugin::~LJCSPlugin() {
    delete (LJCSPluginImpl*)impl_;
    impl_ = 0;
}

bool LJCSPlugin::on_create() {
    assert( impl_ != 0 );
    LJCSPluginImpl& impl = *(LJCSPluginImpl*)impl_;

    impl.create();
    return true;
}

void LJCSPlugin::on_destroy() {
    assert( impl_ != 0 );
    LJCSPluginImpl& impl = *(LJCSPluginImpl*)impl_;

    impl.destroy();
}

