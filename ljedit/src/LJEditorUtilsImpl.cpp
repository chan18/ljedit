// LJEditorUtilsImpl.cpp
//

#include "LJEditorUtilsImpl.h"

#include <fstream>
#include <sys/stat.h>

#include <gtksourceviewmm/sourceview.h>

#include "LanguageManager.h"


typedef std::list<std::string> TStrCoderList;

TStrCoderList init_coders() {
	TStrCoderList coders;
	coders.push_back("utf-8");
	coders.push_back("gb18030");
	return coders;
}

Gtk::TextView* LJEditorUtilsImpl::do_create_source_view(bool highlight_current_line, bool show_line_number) {
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
    gtksourceview::SourceView* view = new gtksourceview::SourceView(buffer);
    if( view != 0 ) {
		
#ifndef WIN32
		view->modify_font(Pango::FontDescription("monospace"));
#endif

        view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_tabs_width(4);
		view->set_highlight_current_line(highlight_current_line);
		view->set_show_line_numbers(show_line_number);
    }

    return view;
}

void LJEditorUtilsImpl::do_destroy_source_view(Gtk::TextView* view) {
    delete view;
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

