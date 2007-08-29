// ljed_mini_cmd.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "IPlugin.h"

class LJMiniCmd : public IPlugin {
public:
    LJMiniCmd(LJEditor& editor) : IPlugin(editor)
		, toolbar_(0)
		, toolitem_(0)
		, mini_cmd_entry_(0)
		, state_('f') {}

    virtual const char* get_plugin_name() { return "LJMiniCmd"; }

protected:
    virtual bool on_create(const char* plugin_filename)  {
		// toolbar
		Glib::RefPtr<Gtk::UIManager> ui = editor().main_window().ui_manager();
		toolbar_ = dynamic_cast<Gtk::Toolbar*>(ui->get_widget("/ToolBar"));
		if( toolbar_==0 )
			return false;

		Gtk::Label* label = Gtk::manage(new Gtk::Label("cmd:"));
		mini_cmd_entry_ = Gtk::manage(new Gtk::Entry());
		
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
		hbox->pack_start(*label);
		hbox->pack_start(*mini_cmd_entry_);

		toolitem_ = Gtk::manage(new Gtk::ToolItem());
		toolitem_->add(*hbox);
		toolitem_->show_all();

		mini_cmd_sigs_.push_back( mini_cmd_entry_->signal_changed().connect(sigc::mem_fun(this, &LJMiniCmd::on_mini_cmd_key_changed)) );
		mini_cmd_sigs_.push_back( mini_cmd_entry_->signal_key_press_event().connect(sigc::mem_fun(this, &LJMiniCmd::on_mini_cmd_key_press), false) );

		toolbar_->append(*toolitem_);

		// menu
		action_group_ = Gtk::ActionGroup::create("MiniCmdActions");

		action_group_->add( Gtk::Action::create("MiniCmdFind", Gtk::Stock::FIND, "_find",  "active mini command control")
			, Gtk::AccelKey("<control>K")
			, sigc::mem_fun(this, &LJMiniCmd::on_find_active) );

		action_group_->add( Gtk::Action::create("MiniCmdGoto", Gtk::Stock::JUMP_TO, "_goto",  "active mini command control")
			, Gtk::AccelKey("<control>I")
			, sigc::mem_fun(this, &LJMiniCmd::on_goto_active) );

		// add menu and toolbar
		ui->insert_action_group(action_group_);

		Glib::ustring ui_info = 
			"<ui>"
			"    <menubar name='MenuBar'>"
			"        <menu action='EditMenu'>"
			"            <menuitem action='MiniCmdFind'/>"
			"            <menuitem action='MiniCmdGoto'/>"
			"        </menu>"
			"    </menubar>"
			"</ui>";

		menu_id_ = ui->add_ui_from_string(ui_info);

		return true;
    }

    virtual void on_destroy() {
		toolbar_->remove(*toolitem_);

		Glib::RefPtr<Gtk::UIManager> ui = editor().main_window().ui_manager();

		ui->remove_action_group(action_group_);
		ui->remove_ui(menu_id_);

		std::for_each( mini_cmd_sigs_.begin()
			, mini_cmd_sigs_.end()
			, std::mem_fun_ref(&sigc::connection::disconnect) );
		mini_cmd_sigs_.clear();
    }

private:
	void on_find_active();
	void on_goto_active();

	void on_mini_cmd_key_changed();
	bool on_mini_cmd_key_press(GdkEventKey* event);
	void on_editing_done();

private:
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;
	Gtk::UIManager::ui_merge_id		menu_id_;
	Gtk::Toolbar*					toolbar_;
	Gtk::ToolItem*					toolitem_;
	Gtk::Entry*						mini_cmd_entry_;
	std::list<sigc::connection>		mini_cmd_sigs_;

	char							state_;
};

void LJMiniCmd::on_find_active() {
	state_ = 'f';
	assert( mini_cmd_entry_!=0 );

	DocPage* page = editor().main_window().doc_manager().get_current_document();
	if( page==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();

	Gtk::TextIter ps, pe;
	if( buf->get_selection_bounds(ps, pe) )
		mini_cmd_entry_->set_text( buf->get_text(ps, pe) );

	mini_cmd_entry_->select_region(0, mini_cmd_entry_->get_text_length());
	mini_cmd_entry_->grab_focus();
}

void LJMiniCmd::on_goto_active() {
	state_ = 'g';
	assert( mini_cmd_entry_!=0 );

	DocPage* page = editor().main_window().doc_manager().get_current_document();
	if( page==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();

	Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());
	if( it != buf->end() ) {
		char buf[64] = { '\0' };
		sprintf(buf, "%d", it.get_line() + 1);
		mini_cmd_entry_->set_text(buf);
	}

	mini_cmd_entry_->select_region(0, mini_cmd_entry_->get_text_length());
	mini_cmd_entry_->grab_focus();
}

void LJMiniCmd::on_mini_cmd_key_changed() {
	DocPage* page = editor().main_window().doc_manager().get_current_document();
	if( page==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();

	assert( mini_cmd_entry_!=0 );

	Glib::ustring text = mini_cmd_entry_->get_text();

	switch( state_ ) {
	case 'g':
		{
			int line_num = atoi(text.c_str());
			if( line_num > 0 ) {
				Gtk::TextIter it = buf->get_iter_at_line(line_num - 1);
				if( it != buf->end() ) {
					buf->place_cursor(it);
					page->view().scroll_to_iter(it, 0.25);
				}
			}
		}
		break;

	default:
		{
			Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());

			Gtk::TextIter ps, pe;
			if( it.forward_search(text, Gtk::TEXT_SEARCH_TEXT_ONLY, ps, pe) ) {
				buf->place_cursor(pe);
				buf->select_range(ps, pe);
				page->view().scroll_to_iter(ps, 0.25);
			}
		}
		break;
	}
}

bool LJMiniCmd::on_mini_cmd_key_press(GdkEventKey* event) {
	DocPage* page = editor().main_window().doc_manager().get_current_document();
	if( page==0 )
		return false;

    switch( event->keyval ) {
    case GDK_Up:
		if( state_!='g' ) {
			Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();

			assert( mini_cmd_entry_!=0 );
			Glib::ustring text = mini_cmd_entry_->get_text();

			Gtk::TextIter it = buf->get_insert()->get_iter();

			Gtk::TextIter ps, pe;
			if( !it.backward_search(text, Gtk::TEXT_SEARCH_TEXT_ONLY, ps, pe) ) {
				it.forward_to_end();

				if( !it.backward_search(text, Gtk::TEXT_SEARCH_TEXT_ONLY, ps, pe) )
					return true;
			}

			buf->select_range(ps, pe);
			page->view().scroll_to_iter(ps, 0.3);
		}
		return true;

    case GDK_Down:
		if( state_!='g' ) {
			Glib::RefPtr<gtksourceview::SourceBuffer> buf = page->buffer();

			assert( mini_cmd_entry_!=0 );
			Glib::ustring text = mini_cmd_entry_->get_text();

			Gtk::TextIter it = buf->get_insert()->get_iter();
			if( !it.forward_char() )
				it.set_offset(0);

			Gtk::TextIter ps, pe;
			if( !it.forward_search(text, Gtk::TEXT_SEARCH_TEXT_ONLY, ps, pe) ) {
				it.set_offset(0);

				if( !it.forward_search(text, Gtk::TEXT_SEARCH_TEXT_ONLY, ps, pe) )
					return true;
			}

			buf->select_range(ps, pe);
			page->view().scroll_to_iter(ps, 0.3);
		}
		return true;

    case GDK_Escape:
    case GDK_Return:
		page->view().grab_focus();
		return true;
    }

	return false;
}

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return new LJMiniCmd(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

