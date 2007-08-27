// DocPageImpl.cpp
// 

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
	Gtk::Button* close_button = Gtk::manage(new Gtk::Button());
	Gtk::Image* img = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU));
	close_button->add(*img);
	close_button->set_size_request(13, 13);
	close_button->set_relief(Gtk::RELIEF_NONE);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
	hbox->pack_start(*label);
	hbox->pack_start(*close_button);
	hbox->show_all();

	// view
    gtksourceview::SourceView* view = LJEditorUtilsImpl::self().create_gtk_source_view();
    if( view == 0 )
		return 0;

	Glib::RefPtr<gtksourceview::SourceLanguage> lang = LJEditorUtilsImpl::self().get_source_language_manager()->get_language_from_mime_type("text/x-c++hdr");
	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = view->get_source_buffer();
	buffer->set_language(lang);
	buffer->set_highlight();

#ifndef WIN32
	view->modify_font(Pango::FontDescription("Courier 10 Pitch"));
#endif

	view->set_highlight_current_line(true);
    view->set_wrap_mode(Gtk::WRAP_NONE);
    view->set_tabs_width(4);
	view->set_auto_indent(true);

    DocPageImpl* page = Gtk::manage(new DocPageImpl(*label, *close_button, *hbox, *view));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

    return page;
}

