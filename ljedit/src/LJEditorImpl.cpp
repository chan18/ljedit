// LJEditorImpl.cpp
//

#include "LJEditorImpl.h"

#include "PluginManager.h"
#include "LanguageManager.h"

#include "LJEditorPythonPluginEngine.h"


LJEditorImpl::LJEditorImpl() : main_window__(0) {
}

LJEditorImpl::~LJEditorImpl() {
}

bool LJEditorImpl::create(const std::string& path) {
	// create MainWindow use glade file
	// 
	main_window__ = MainWindowImpl::create__(path);

    try {
        main_window_.create(path);
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


Gtk::TextView* LJEditorImpl::create_source_view() {
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
    gtksourceview::SourceView* view = new gtksourceview::SourceView(buffer);
    if( view != 0 ) {
        view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_highlight_current_line();
    }
    return view;
}

void LJEditorImpl::destroy_source_view(Gtk::TextView* view) {
    delete view;
}

