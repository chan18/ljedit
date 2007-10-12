// LJEditorUtilsImpl.cpp
//

#include "StdAfx.h"	// for vc precompile header

#include "LJEditorUtilsImpl.h"

#include <sys/stat.h>
#include <fstream>
#include <sstream>

#include <gtksourceview/gtksourcelanguage.h>

typedef std::list<std::string> TStrCoderList;

TStrCoderList init_coders() {
	TStrCoderList coders;
	coders.push_back("utf-8");
	coders.push_back("gb18030");
	return coders;
}

LJEditorUtilsImpl::LJEditorUtilsImpl() {
#ifdef WIN32
	//font_ = "simsun 9";
#else
	font_ = "Courier 10 Pitch 10";
#endif
}

LJEditorUtilsImpl::~LJEditorUtilsImpl() {
}

void LJEditorUtilsImpl::create(const std::string& path) {
	// get language map
	//
	Glib::RefPtr<gtksourceview::SourceLanguageManager> mgr = gtksourceview::SourceLanguageManager::get_default();
	
	Glib::StringArrayHandle ids = mgr->get_language_ids();
	Glib::StringArrayHandle::iterator it = ids.begin();
	Glib::StringArrayHandle::iterator end = ids.end();
	for( ; it!=end; ++it ) {
		Glib::RefPtr<gtksourceview::SourceLanguage> lang = mgr->get_language(*it);
		if( !lang )
			continue;
		
		// TODO : use regex
		// 
		Glib::StringArrayHandle globs = gtk_source_language_get_globs(lang->gobj());
		Glib::StringArrayHandle::iterator ps = globs.begin();
		Glib::StringArrayHandle::iterator pe = globs.end();
		for( ; ps!=pe; ++ps ) {
			Glib::ustring glob = *ps;
			if( glob.size() > 2 && glob[0]=='*' && glob[1]=='.' ) {
				glob.erase(0, 2);
				language_map_[glob] = lang->get_id();
			} else {
				// TODO : now not finish
				language_map_[glob] = lang->get_id();
			}
			//printf("%s:%s\n", glob.c_str(), lang->get_id().c_str());
		}
	}
}

gtksourceview::SourceView* LJEditorUtilsImpl::create_gtk_source_view() {
    gtksourceview::SourceView* view = new gtksourceview::SourceView();
    if( view != 0 ) {
		view->modify_font(Pango::FontDescription(font_));
		view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_tab_width(4);
    }

	return view;
}

void LJEditorUtilsImpl::destroy_gtk_source_view(gtksourceview::SourceView* view) {
	delete view;
}

const Glib::ustring& LJEditorUtilsImpl::get_language_id_by_filename(const Glib::ustring& filename) {
	static Glib::ustring default_retval = "";

	// TODO : now implement not finish

	Glib::ustring name = filename;
	{
		size_t pos = name.find_last_of("/.");
		if( pos != name.npos ) {
			name.erase(0, pos+1);
		}
	}

	std::map<Glib::ustring, Glib::ustring>::iterator it = language_map_.find(name);
	if( it != language_map_.end() )
		return it->second;

	return default_retval;
}

bool LJEditorUtilsImpl::do_load_file(Glib::ustring& out, const std::string& filename) {
	//if( !Glib::file_test(filename, Glib::FILE_TEST_EXISTS) )
	//	return false;

	Glib::ustring buf;
	Glib::ustring ubuf;

	try {
		buf = Glib::file_get_contents(filename);

	} catch(const Glib::FileError&) {
		return false;
	}

	if( ::g_utf8_validate(buf.c_str(), buf.size(), 0) ) {
		ubuf = buf;

	} else {
		try {
			ubuf = Glib::locale_to_utf8(buf);

		} catch(const Glib::ConvertError&) {
			static TStrCoderList coders = init_coders();

			TStrCoderList::const_iterator it = coders.begin();
			TStrCoderList::const_iterator end = coders.end();
			for( ; it!=end; ++it ) {
				try {
					ubuf = Glib::convert(buf, "utf-8", *it);
					break;
				} catch(const Glib::ConvertError&) {
				}
			}

			if( it==end )
				return false;
		}
	}

	out.swap(ubuf);
	return true;
}

