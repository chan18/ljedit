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

	virtual gtksourceview::SourceView* create_gtk_source_view();

	virtual void destroy_gtk_source_view(gtksourceview::SourceView* view);

private:
    LJEditorUtilsImpl() {}
    ~LJEditorUtilsImpl() {}

    LJEditorUtilsImpl(const LJEditorUtilsImpl&);
    LJEditorUtilsImpl& operator = (const LJEditorUtilsImpl&);

private:
	virtual Glib::RefPtr<gtksourceview::SourceLanguagesManager> do_get_source_language_manager();

	virtual bool do_load_file(Glib::ustring& out, const std::string& filename);
};

#endif//LJED_INC_LJEDITORUTILSIMPL_H

