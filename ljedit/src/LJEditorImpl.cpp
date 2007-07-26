// LJEditorImpl.cpp
//

#include "LJEditorImpl.h"

#include "PluginManager.h"

#include "LJEditorPythonPluginEngine.h"

bool LJEditorImpl::create() {
    try {
        main_window_.create();
    } catch( const Glib::Exception& e ) {
        Gtk::MessageDialog dlg(e.what(), false, Gtk::MESSAGE_ERROR);
        dlg.set_title("LJEdit ERROR");
        dlg.run();
        return false;
    }

    PluginManager::self().load_plugins();

	::ljed_start_python_plugin_engine();

    return true;
}

void LJEditorImpl::destroy() {
	::ljed_stop_python_plugin_engine();

    PluginManager::self().unload_plugins();
}

void LJEditorImpl::run() {
    try {
        Gtk::Main::run(main_window_);
    } catch( const Glib::Exception& e ) {
        Gtk::MessageDialog dlg(e.what(), false, Gtk::MESSAGE_ERROR);
        dlg.set_title("LJEdit ERROR");
        dlg.run();
    }
}

