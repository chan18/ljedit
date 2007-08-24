// LJEditorUtilsImpl.cpp
//

#include "LJEditorUtilsImpl.h"

#include <fstream>
#include <sys/stat.h>

#include <gtksourceviewmm/init.h>

#include <gtksourceviewmm/sourcebuffer.h>


Glib::RefPtr<gtksourceview::SourceLanguagesManager> create_source_language_manager() {
    gtksourceview::init();

    return gtksourceview::SourceLanguagesManager::create();
}


typedef std::list<std::string> TStrCoderList;

TStrCoderList init_coders() {
	TStrCoderList coders;
	coders.push_back("utf-8");
	coders.push_back("gb18030");
	return coders;
}

gtksourceview::SourceView* LJEditorUtilsImpl::create_gtk_source_view() {
    gtksourceview::SourceView* view = new gtksourceview::SourceView();
    if( view != 0 ) {
		
#ifndef WIN32
		view->modify_font(Pango::FontDescription("Courier 10 Pitch"));
#endif

		view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_tabs_width(4);
    }

	return view;
}

void LJEditorUtilsImpl::destroy_gtk_source_view(gtksourceview::SourceView* view) {
	delete view;
}

Glib::RefPtr<gtksourceview::SourceLanguagesManager> LJEditorUtilsImpl::do_get_source_language_manager() {
	static Glib::RefPtr<gtksourceview::SourceLanguagesManager> mgr = create_source_language_manager();
	return mgr;
}

bool LJEditorUtilsImpl::do_load_file(Glib::ustring& out, const std::string& filename) {
	struct stat st;
	::memset(&st, 0, sizeof(st));

	if( ::stat(filename.c_str(), &st)!=0 )
		return false;

	std::string buf;
	try {
		buf.resize(st.st_size);

		std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary);
		ifs.read(&buf[0], (std::streamsize)buf.size());

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

