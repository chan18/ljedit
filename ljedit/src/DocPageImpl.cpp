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

    gtksourceview::SourceView* view = Gtk::manage(new gtksourceview::SourceView(buffer));
    view->set_wrap_mode(Gtk::WRAP_NONE);
	view->set_tabs_width(4);
    view->set_highlight_current_line();
        
    DocPageImpl* page = Gtk::manage(new DocPageImpl(*label, *view));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

    return page;
}

