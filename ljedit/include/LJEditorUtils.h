// LJEditorUtils.h
// 

#ifndef LJED_INC_LJEDITORUTILS_H
#define LJED_INC_LJEDITORUTILS_H

#include "gtkenv.h"

class LJEditorUtils {
public:
    Gtk::TextView* create_source_view( bool highlight_current_line=true, bool show_line_number=false )
		{ return do_create_source_view(highlight_current_line, show_line_number); }

	void destroy_source_view(Gtk::TextView* view)
		{ return do_destroy_source_view(view); }

private:
    virtual Gtk::TextView* do_create_source_view(bool highlight_current_line, bool show_line_number) = 0;

    virtual void do_destroy_source_view(Gtk::TextView* view) = 0;
};

#endif//LJED_INC_LJEDITORUTILS_H

