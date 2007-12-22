// LJEditorUtilsImpl.h
//

#ifndef LJED_INC_LJEDITORUTILSIMPL_H
#define LJED_INC_LJEDITORUTILSIMPL_H

#include "LJEditorUtils.h"

#include <vector>
#include <map>

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

	virtual void format_filekey(std::string& filename);

private:
    LJEditorUtilsImpl();
    ~LJEditorUtilsImpl();

    LJEditorUtilsImpl(const LJEditorUtilsImpl&);
    LJEditorUtilsImpl& operator = (const LJEditorUtilsImpl&);

	void on_option_changed(const std::string& id, const std::string& value, const std::string& old);

	void option_set_loader_charset_order(const std::string& option_text);

private:
	virtual bool do_load_file(Glib::ustring& contents_out, Glib::ustring& charset_out, const std::string& filename);

private:
	std::map<Glib::ustring, Glib::ustring>	language_map_;

	std::vector<std::string>				option_loader_charset_order_;
};

#endif//LJED_INC_LJEDITORUTILSIMPL_H

