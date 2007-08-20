// LJEditorUtilsImpl.h
//

#ifndef LJED_INC_LJEDITORUTILSIMPL_H
#define LJED_INC_LJEDITORUTILSIMPL_H

#include "LJEditorUtils.h"

class LJEditorUtilsImpl : public LJEditorUtils {
public:
    static LJEditorUtilsImpl& self() {
        static LJEditorUtilsImpl me;
        return me;
    }

private:
    LJEditorUtilsImpl() {}
    ~LJEditorUtilsImpl() {}

    LJEditorUtilsImpl(const LJEditorUtilsImpl&);
    LJEditorUtilsImpl& operator = (const LJEditorUtilsImpl&);

private:
	virtual Gtk::TextView* create_source_view(bool show_line_number=false) ;
	virtual void destroy_source_view(Gtk::TextView* view);
};

#endif//LJED_INC_LJEDITORUTILSIMPL_H

