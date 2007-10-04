// DocPageImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "DocPageImpl.h"

#include <fstream>

#include "LJEditorUtilsImpl.h"
#include "dir_utils.h"

DocPageImpl* DocPageImpl::create(const std::string& filepath
        , const std::string& display_name)
{
	Glib::ustring u_display_name;
	try {
		u_display_name = Glib::locale_to_utf8(display_name);
	} catch(const Glib::Exception&) {
		u_display_name = display_name;
	}

    Gtk::Label* label = Gtk::manage(new Gtk::Label(u_display_name));
	Gtk::EventBox* label_event_box = Gtk::manage(new Gtk::EventBox());
	label_event_box->add(*label);
	label_event_box->show_all();

	// view
    gtksourceview::SourceView* view = LJEditorUtilsImpl::self().create_gtk_source_view();
    if( view == 0 )
		return 0;
	view->set_show_line_numbers();

	Glib::RefPtr<gtksourceview::SourceLanguagesManager> mgr = LJEditorUtilsImpl::self().get_source_language_manager();
	Glib::RefPtr<gtksourceview::SourceLanguage> lang;

	// TODO : get mime-type form file .xxx name
	{
		size_t pos = display_name.find_last_of('.');
		if( pos != display_name.npos ) {
			std::string filetype = display_name.substr(pos);
			if( filetype==".py" ) {
				lang = mgr->get_language_from_mime_type("text/x-python");

			} else if( filetype==".xml" ) {
				lang = mgr->get_language_from_mime_type("text/xml");
			}
		}
	}

	if( !lang )
		lang = mgr->get_language_from_mime_type("text/x-c++hdr");

	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = view->get_source_buffer();
	buffer->set_language(lang);
	buffer->set_highlight();
	buffer->set_max_undo_levels(1024);

	view->set_highlight_current_line(true);
    view->set_wrap_mode(Gtk::WRAP_NONE);
    view->set_tabs_width(4);
	view->set_auto_indent(true);

    DocPageImpl* page = Gtk::manage(new DocPageImpl(*label, *label_event_box, *view));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

    return page;
}

