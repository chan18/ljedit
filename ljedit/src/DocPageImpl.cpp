// DocPageImpl.cpp
// 

#include "DocPageImpl.h"

#include <fstream>

#include "LanguageManager.h"
#include "dir_utils.h"

DocPageImpl* DocPageImpl::create(const std::string& filepath
        , const std::string& display_name
        , Glib::RefPtr<gtksourceview::SourceBuffer> buffer)
{
    Gtk::Label* label = Gtk::manage(new Gtk::Label(display_name));
	Gtk::Button* close_button = Gtk::manage(new Gtk::Button());
	Gtk::Image* img = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU));
	close_button->add(*img);
	close_button->set_size_request(13, 13);
	close_button->set_relief(Gtk::RELIEF_NONE);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
	hbox->pack_start(*label);
	hbox->pack_start(*close_button);
	hbox->show_all();

    gtksourceview::SourceView* view = Gtk::manage(new gtksourceview::SourceView(buffer));

#ifndef WIN32
	view->modify_font(Pango::FontDescription("monospace"));
#endif

	view->set_wrap_mode(Gtk::WRAP_NONE);
	view->set_tabs_width(4);
    view->set_highlight_current_line();
        
    DocPageImpl* page = Gtk::manage(new DocPageImpl(*label, *close_button, *hbox, *view));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

    return page;
}

