// LJEditorUtilsImpl.cpp
//

#include "LJEditorUtilsImpl.h"

#include "LanguageManager.h"

#include <gtksourceviewmm/sourceview.h>

Gtk::TextView* LJEditorUtilsImpl::do_create_source_view(bool highlight_current_line, bool show_line_number) {
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
    gtksourceview::SourceView* view = new gtksourceview::SourceView(buffer);
    if( view != 0 ) {
		
#ifndef WIN32
		view->modify_font(Pango::FontDescription("monospace"));
#endif

        view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_tabs_width(4);
		view->set_highlight_current_line(highlight_current_line);
		view->set_show_line_numbers(show_line_number);
    }

    return view;
}

void LJEditorUtilsImpl::do_destroy_source_view(Gtk::TextView* view) {
    delete view;
}

