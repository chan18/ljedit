// LJEditorUtils.h
// 

#ifndef LJED_INC_LJEDITORUTILS_H
#define LJED_INC_LJEDITORUTILS_H

#include "gtkenv.h"

class LJEditorUtils {
public:
	Glib::RefPtr<gtksourceview::SourceLanguagesManager> get_source_language_manager()
		{ return do_get_source_language_manager(); }

	virtual gtksourceview::SourceView* create_gtk_source_view() = 0;

	virtual void destroy_gtk_source_view(gtksourceview::SourceView* view) = 0;

	// file loader
	bool load_file(Glib::ustring& out, const std::string& filename)
		{ return do_load_file(out, filename); }

private:
	virtual Glib::RefPtr<gtksourceview::SourceLanguagesManager> do_get_source_language_manager() = 0;

	virtual bool do_load_file(Glib::ustring& out, const std::string& filename) = 0;
};

#endif//LJED_INC_LJEDITORUTILS_H

