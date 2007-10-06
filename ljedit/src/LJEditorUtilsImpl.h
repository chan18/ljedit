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

    void create(const std::string& path);

	virtual gtksourceview::SourceView* create_gtk_source_view();

	virtual void destroy_gtk_source_view(gtksourceview::SourceView* view);

	virtual const Glib::ustring& get_language_id_by_filename(const Glib::ustring& filename);

	const Glib::ustring& font() const	{ return font_; }
	Glib::ustring& font()				{ return font_; }

private:
    LJEditorUtilsImpl();
    ~LJEditorUtilsImpl();

    LJEditorUtilsImpl(const LJEditorUtilsImpl&);
    LJEditorUtilsImpl& operator = (const LJEditorUtilsImpl&);

private:
	virtual bool do_load_file(Glib::ustring& out, const std::string& filename);

private:
	Glib::ustring font_;
	
	std::map<Glib::ustring, Glib::ustring>	language_map_;
};

#endif//LJED_INC_LJEDITORUTILSIMPL_H

