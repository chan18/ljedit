// ljed_indent.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "IPlugin.h"

class LJIndent : public IPlugin {
public:
    LJIndent(LJEditor& editor) : IPlugin(editor) {}

    virtual const char* get_plugin_name() { return "LJIndent"; }

protected:
    virtual bool on_create(const char* plugin_filename)  {
		// menu
		action_group_ = Gtk::ActionGroup::create("IndentActions");

		action_group_->add( Gtk::Action::create("Indent", Gtk::Stock::INDENT, "_indent",  "indent selected lines")
			, Gtk::AccelKey("<control>T")
			, sigc::mem_fun(this, &LJIndent::on_indent) );

		action_group_->add( Gtk::Action::create("Unindent", Gtk::Stock::UNINDENT, "u_nindent", "uindent selected lines")
			, Gtk::AccelKey("<Shift><Control>T")
			, sigc::mem_fun(this, &LJIndent::on_unindent) );

		Glib::ustring ui_info = 
			"<ui>"
			"    <menubar name='MenuBar'>"
			"        <menu action='EditMenu'>"
			"            <menuitem action='Indent'/>"
			"            <menuitem action='Unindent'/>"
			"        </menu>"
			"    </menubar>"
			"</ui>";

		editor().main_window().ui_manager()->insert_action_group(action_group_);
		menu_id_ = editor().main_window().ui_manager()->add_ui_from_string(ui_info);

        return true;
    }

    virtual void on_destroy() {
		editor().main_window().ui_manager()->remove_action_group(action_group_);
		editor().main_window().ui_manager()->remove_ui(menu_id_);
    }

private:
	void on_indent();

	void on_unindent();

private:
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;
	Gtk::UIManager::ui_merge_id		menu_id_;
};

void LJIndent::on_indent() {
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

void LJIndent::on_unindent() {
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

		Gtk::TextIter sit;
		Gtk::TextIter eit;

		for( gint i=sline; i<=eline; ++i ) {
			sit = buf->get_iter_at_line(i);

			if( sit.get_char()=='\t' ) {
				eit = sit;
				eit.forward_char();
				buf->erase(sit, eit);

			} else if( sit.get_char()==' ' ) {
				gint spaces = 0;
				eit = sit;
				while( !eit.ends_line() ) {
					if( eit.get_char()==' ' )
						++spaces;
					else
						break;

					eit.forward_char();
				}

				if( spaces > 0 ) {
					guint tabs_size = page->view().get_tabs_width();
					guint tabs = spaces / tabs_size;
					spaces -= (tabs * tabs_size);

					if (spaces == 0)
						spaces = tabs_size;

					eit = sit;
					eit.forward_chars(spaces);
					buf->erase(sit, eit);
				}
			}
		}
	}
	buf->end_user_action();
}

LJED_PLUGIN_DLL_EXPORT IPlugin* plugin_create(LJEditor& editor) {
    return new LJIndent(editor);
}

LJED_PLUGIN_DLL_EXPORT void plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

