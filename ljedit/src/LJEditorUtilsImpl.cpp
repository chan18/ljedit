// LJEditorUtilsImpl.cpp
//

#include "LJEditorUtilsImpl.h"

#include "LanguageManager.h"

#include <gtksourceviewmm/sourceview.h>

Gtk::TextView* LJEditorUtilsImpl::create_source_view(bool show_line_number) {
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
    gtksourceview::SourceView* view = new gtksourceview::SourceView(buffer);
    if( view != 0 ) {
        view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_tabs_width(4);
		view->set_highlight_current_line();
    }
    return view;
}

void LJEditorUtilsImpl::destroy_source_view(Gtk::TextView* view) {
    delete view;
}

