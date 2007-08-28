// ljed_mini_cmd.cpp
// 

#include "IPlugin.h"

class LJMiniCmd : public IPlugin {
public:
    LJMiniCmd(LJEditor& editor) : IPlugin(editor)
		, mini_cmd_item_(0)
		, mini_cmd_entry_(0) {}

    virtual const char* get_plugin_name() { return "LJMiniCmd"; }

protected:
    virtual bool on_create(const char* plugin_filename)  {
		// toolbar
		Glib::RefPtr<Gtk::UIManager> ui = editor().main_window().ui_manager();
		Gtk::Toolbar* toolbar = dynamic_cast<Gtk::Toolbar*>(ui->get_widget("/ToolBar"));
		if( toolbar==0 )
			return false;

		Gtk::Label* label = Gtk::manage(new Gtk::Label("cmd:"));
		Gtk::ComboBoxEntry* entry = Gtk::manage(new Gtk::ComboBoxEntry());
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
		hbox->pack_start(*label);
		hbox->pack_start(*entry);

		Gtk::ToolItem* item = Gtk::manage(new Gtk::ToolItem());
		item->add(*hbox);
		item->show_all();

		item->signal_key_release_event().connect( sigc::mem_fun(this, &LJMiniCmd::on_mini_cmd_key_release) );

		toolbar->append(*item);

		// menu
		action_group_ = Gtk::ActionGroup::create("MiniCmdActions");

		action_group_->add( Gtk::Action::create("MiniCmd", Gtk::Stock::INDENT, "_minicmd",  "active mini command control")
			, Gtk::AccelKey("<control>K")
			, sigc::mem_fun(this, &LJMiniCmd::on_mini_cmd_active) );

		// add menu and toolbar
		ui->insert_action_group(action_group_);

		Glib::ustring ui_info = 
			"<ui>"
			"    <menubar name='MenuBar'>"
			"        <menu action='EditMenu'>"
			"            <menuitem action='MiniCmd'/>"
			"        </menu>"
			"    </menubar>"
			"</ui>";

		menu_id_ = ui->add_ui_from_string(ui_info);

		return true;
    }

    virtual void on_destroy() {
		Glib::RefPtr<Gtk::UIManager> ui = editor().main_window().ui_manager();

		ui->remove_action_group(action_group_);
		ui->remove_ui(menu_id_);

		Gtk::Toolbar* toolbar = dynamic_cast<Gtk::Toolbar*>(ui->get_widget("/ToolBar"));
		if( toolbar==0 )
			return;

		if( mini_cmd_item_!=0 )
			toolbar->remove(*mini_cmd_item_);
    }

private:
	void on_mini_cmd_active();
	bool on_mini_cmd_key_release(GdkEventKey* event);

private:
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;
	Gtk::UIManager::ui_merge_id		menu_id_;
	Gtk::ToolItem*					mini_cmd_item_;
	Gtk::ComboBoxEntry*				mini_cmd_entry_;

};

void LJMiniCmd::on_mini_cmd_active() {
	assert( mini_cmd_entry_!=0 && mini_cmd_entry_->get_entry()!=0 );

	Gtk::Entry* entry = mini_cmd_entry_->get_entry();
	entry->select_region(0, entry->get_text_length());
	mini_cmd_entry_->grab_focus();
}

bool LJMiniCmd::on_mini_cmd_key_release(GdkEventKey* event) {
	DocPage* page = editor().main_window().doc_manager().get_current_document();
	if( page==0 )
		return false;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();

	assert( mini_cmd_entry_!=0 && mini_cmd_entry_->get_entry()!=0 );

	Gtk::Entry* entry = mini_cmd_entry_->get_entry();
	Glib::ustring text = entry->get_text();

	Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());


	return false;	
}

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return new LJMiniCmd(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

