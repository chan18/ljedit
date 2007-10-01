// ljed_hello.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "IPlugin.h"

class LJHello : public IPlugin {
public:
    LJHello(LJEditor& editor) : IPlugin(editor) {}

    virtual const char* get_plugin_name() { return "LJHello"; }

protected:
    virtual bool on_create(const char* plugin_filename) {
        /*
        Gtk::MessageDialog dlg(__FUNCTION__, false, Gtk::MESSAGE_INFO);
        dlg.set_title("LJHello");
        dlg.run();
        */
		
		// menu
		action_group_ = Gtk::ActionGroup::create("HelloActions");
		action_group_->add( Gtk::Action::create("HelloAbout", Gtk::Stock::ABOUT,   "_hello",  "Hello plugin"),	sigc::mem_fun(this, &LJHello::on_about) );

		Glib::ustring ui_info = 
			"<ui>"
			"    <menubar name='MenuBar'>"
			"        <menu action='PluginsMenu'>"
			"            <menuitem action='HelloAbout'/>"
			"        </menu>"
			"    </menubar>"
			"</ui>";

		editor().main_window().ui_manager()->insert_action_group(action_group_);
		menu_id_ = editor().main_window().ui_manager()->add_ui_from_string(ui_info);

        return true;
    }

    virtual void on_destroy() {
        /*
        Gtk::MessageDialog dlg(__FUNCTION__, false, Gtk::MESSAGE_INFO);
        dlg.set_title("LJHello");
        dlg.run();
        */
		
		editor().main_window().ui_manager()->remove_action_group(action_group_);
		editor().main_window().ui_manager()->remove_ui(menu_id_);
    }

private:
	void on_about() {
        Gtk::MessageDialog dlg("welcome to use LJHello plugin!", false, Gtk::MESSAGE_INFO);
        dlg.set_title("LJHello");
        dlg.run();
	}

private:
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;
	Gtk::UIManager::ui_merge_id		menu_id_;
};

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return new LJHello(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

