// LJEditorUtils.h
// 

#ifndef LJED_INC_LJEDITORUTILS_H
#define LJED_INC_LJEDITORUTILS_H

#include "gtkenv.h"

class LJEditorUtils {
protected:
	virtual ~LJEditorUtils() {}

public:
	virtual gtksourceview::SourceView* create_gtk_source_view() = 0;

	virtual void destroy_gtk_source_view(gtksourceview::SourceView* view) = 0;

	// filetype : cpp, cc, c, hpp, hh, h, py, php, java, lua, ....
	// 
	virtual const Glib::ustring& get_language_id_by_filename(const Glib::ustring& filename) = 0;

	const Glib::RefPtr<gtksourceview::SourceLanguage> get_language_by_filename(const Glib::ustring& filename) {
		const Glib::ustring& id = get_language_id_by_filename(filename);
		return gtksourceview::SourceLanguageManager::get_default()->get_language(id);
	}

	// file loader
	bool load_file(Glib::ustring& out, const std::string& filename)
		{ return do_load_file(out, filename); }

	// filekey is unique filename in ljedit
	// 
	virtual void format_filekey(std::string& filename) = 0;

private:
	virtual bool do_load_file(Glib::ustring& out, const std::string& filename) = 0;
};

#endif//LJED_INC_LJEDITORUTILS_H

