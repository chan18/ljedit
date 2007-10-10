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
	struct stat st;
	::memset(&st, 0, sizeof(st));

	if( ::stat(filename.c_str(), &st)!=0 )
		return false;

	std::string buf;
	size_t sz = st.st_size;
	try {
		buf.resize(sz);

		FILE* fp = fopen(filename.c_str(), "rb");
		if( fp==0 )
			return false;

		sz = fread(&buf[0], 1, sz, fp);
		fclose(fp);

		//Glib::RefPtr<Glib::IOChannel> ifs = Glib::IOChannel::create_from_file(filename, "r");
		//std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary);
		//if( !ifs )
		//	return false;
		//ifs->read(&buf[0], sz, sz);
		//ifs->read(&buf[0], (std::streamsize)buf.size());

		if( sz != st.st_size )
			return false;

	} catch(const std::exception&) {
		return false;
	}

	Glib::ustring ubuf;
	{
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

