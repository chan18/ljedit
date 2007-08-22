// LJEditorUtilsImpl.h
//

#ifndef LJED_INC_LJEDITORUTILSIMPL_H
#define LJED_INC_LJEDITORUTILSIMPL_H

#include "LJEditorUtils.h"

#include <gtksourceviewmm/sourceview.h>

class LJEditorUtilsImpl : public LJEditorUtils {
public:
    static LJEditorUtilsImpl& self() {
        static LJEditorUtilsImpl me;
        return me;
    }

	gtksourceview::SourceView* create_gtk_source_view(bool highlight_current_line=true, bool show_line_number=false);

	void destroy_gtk_source_view(gtksourceview::SourceView* view) { delete view; }

private:
    LJEditorUtilsImpl() {}
    ~LJEditorUtilsImpl() {}

    LJEditorUtilsImpl(const LJEditorUtilsImpl&);
    LJEditorUtilsImpl& operator = (const LJEditorUtilsImpl&);

private:
	virtual Gtk::TextView* do_create_source_view(bool highlight_current_line, bool show_line_number)
		{ return create_gtk_source_view(highlight_current_line, show_line_number); }

	virtual void do_destroy_source_view(Gtk::TextView* view)
		{ destroy_gtk_source_view(dynamic_cast<gtksourceview::SourceView*>(view)); }

	virtual bool do_load_file(Glib::ustring& out, const std::string& filename);
};

#endif//LJED_INC_LJEDITORUTILSIMPL_H

