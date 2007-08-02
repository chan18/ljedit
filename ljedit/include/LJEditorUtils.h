// LJEditorUtils.h
// 

#ifndef LJED_INC_LJEDITORUTILS_H
#define LJED_INC_LJEDITORUTILS_H

#include "gtkenv.h"

class LJEditorUtils {
public:
    virtual Gtk::TextView* create_source_view() = 0;
    virtual void destroy_source_view(Gtk::TextView* view) = 0;
};

#endif//LJED_INC_LJEDITORUTILS_H

