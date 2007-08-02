// ljed_hello.cpp
// 

#include "IPlugin.h"

class LJHello : public IPlugin {
public:
    LJHello(LJEditor& editor) : IPlugin(editor) {}

    virtual const char* get_plugin_name() { return "LJHello"; }

protected:
    virtual bool on_create(const char* plugin_filename)  {
        /*
        Gtk::MessageDialog dlg(__FUNCTION__, false, Gtk::MESSAGE_INFO);
        dlg.set_title("LJHello");
        dlg.run();
        */
        return true;
    }

    virtual void on_destroy() {
        /*
        Gtk::MessageDialog dlg(__FUNCTION__, false, Gtk::MESSAGE_INFO);
        dlg.set_title("LJHello");
        dlg.run();
        */
    }
};

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return new LJHello(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

