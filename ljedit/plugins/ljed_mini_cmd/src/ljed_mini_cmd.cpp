// ljed_mini_cmd.cpp
// 

#include "IPlugin.h"

class LJMiniCmd : public IPlugin {
public:
    LJMiniCmd(LJEditor& editor) : IPlugin(editor) {}

    virtual const char* get_plugin_name() { return "LJMiniCmd"; }

protected:
    virtual bool on_create(const char* plugin_filename)  {
		// menu
		action_group_ = Gtk::ActionGroup::create("MiniCmdActions");

		action_group_->add( Gtk::Action::create("MiniCmd", Gtk::Stock::INDENT, "_minicmd",  "active mini command control")
			, Gtk::AccelKey("<control>K")
			, sigc::mem_fun(this, &LJMiniCmd::on_active_mini_cmd) );

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

		toolbar->append(*item);

		return true;
    }

    virtual void on_destroy() {
		editor().main_window().ui_manager()->remove_action_group(action_group_);
		editor().main_window().ui_manager()->remove_ui(menu_id_);
    }

private:
	void on_active_mini_cmd();

private:
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;
	Gtk::UIManager::ui_merge_id		menu_id_;
};

void LJMiniCmd::on_active_mini_cmd() {
	DocPage* page = editor().main_window().doc_manager().get_current_document();
	if( page==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();
	buf->begin_user_action();
	{
		Gtk::TextIter start, end;
		gint sline, eline;

		buf->get_selection_bounds(start, end);
		sline = start.get_line();
		eline = end.get_line();

		if( end.get_visible_line_offset()==0 && eline > sline )
			--eline;

		Glib::ustring tab = page->view().get_insert_spaces_instead_of_tabs()
			? Glib::ustring(page->view().get_tabs_width(), ' ')
			: "\t";

		Gtk::TextIter it;

		for( gint i=sline; i<=eline; ++i ) {
			it = buf->get_iter_at_line(i);

			if( it.ends_line() )
				continue;

			buf->insert(it, tab);
		}
	}
	buf->end_user_action();
}

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return new LJMiniCmd(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

