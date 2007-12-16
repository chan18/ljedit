// DocPageImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "DocPageImpl.h"

#include <fstream>

#include "LJEditorUtilsImpl.h"

DocPageImpl* DocPageImpl::create(const std::string& filepath
        , const Glib::ustring& display_name
		, bool& option_use_mouse_double_click)
{
    Gtk::Label* label = Gtk::manage(new Gtk::Label(display_name));
	Gtk::EventBox* label_event_box = Gtk::manage(new Gtk::EventBox());
	label_event_box->add(*label);
	label_event_box->show_all();

	// view
    gtksourceview::SourceView* view = LJEditorUtilsImpl::self().create_gtk_source_view();
    if( view == 0 )
		return 0;
	view->set_show_line_numbers();
	view->set_wrap_mode(Gtk::WRAP_NONE);

	Glib::RefPtr<gtksourceview::SourceLanguage> lang = LJEditorUtilsImpl::self().get_language_by_filename(display_name);
	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = view->get_source_buffer();
	buffer->set_language(lang);
	buffer->set_highlight_syntax();
	buffer->set_max_undo_levels(1024);

	view->set_highlight_current_line(true);
	view->set_auto_indent(true);

    DocPageImpl* page = Gtk::manage(new DocPageImpl(*label, *label_event_box, *view, option_use_mouse_double_click));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

	// mouse double click select
	view->signal_button_press_event().connect( sigc::mem_fun(page, &DocPageImpl::mouse_double_click), false );

    return page;
}


bool DocPageImpl::mouse_double_click(GdkEventButton* event) {
	if( option_use_mouse_double_click_ && event->button==1 && event->type==GDK_2BUTTON_PRESS ) {
		gtksourceview::SourceIter ps = buffer()->get_iter_at_mark(buffer()->get_insert());
		gtksourceview::SourceIter pe = ps;
		
		char ch = (char)ps.get_char();
		if( isalnum(ch)==0 && ch!='_' )
			return false;

		// find word start position
		while( ps.backward_char() ) {
			ch = ps.get_char();
			if( ch > 0 && (::isalnum(ch) || ch=='_') )
				continue;
			ps.forward_char();
			break;
		}

		// find key end position
		while( pe.forward_char() ) {
			ch = pe.get_char();
			if( ch > 0 && (::isalnum(ch) || ch=='_') )
				continue;
			break;
		}

		buffer()->select_range(pe, ps);
		return true;
	}

	return false;
}

