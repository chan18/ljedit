// DocPageImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "DocPageImpl.h"

#include <fstream>

#include "LJEditorUtilsImpl.h"


DocPageImpl* DocPageImpl::create(const std::string& filepath
        , const Glib::ustring& display_name)
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

	Glib::RefPtr<gtksourceview::SourceLanguage> lang = LJEditorUtilsImpl::self().get_language_by_filename(display_name);
	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = view->get_source_buffer();
	buffer->set_language(lang);
	buffer->set_highlight_syntax();
	buffer->set_max_undo_levels(1024);

	view->set_highlight_current_line(true);
    view->set_wrap_mode(Gtk::WRAP_NONE);
    view->set_tab_width(4);
	view->set_auto_indent(true);

    DocPageImpl* page = Gtk::manage(new DocPageImpl(*label, *label_event_box, *view));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

    return page;
}

