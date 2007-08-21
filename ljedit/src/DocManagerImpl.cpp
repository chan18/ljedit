// DocManagerImpl.cpp
// 

#include "DocManagerImpl.h"

#include <fstream>
#include <sys/stat.h>

#include "LanguageManager.h"
#include "dir_utils.h"

DocManagerImpl::DocManagerImpl() : page_num_(-1) {
}

DocManagerImpl::~DocManagerImpl() {
     close_all_files();
}

void DocManagerImpl::create_new_file() {
    static std::string noname = "noname";

    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();

    open_page("", noname, buffer);
}

void DocManagerImpl::locate_page_line(int page_num, int line) {
    if( page_num_ < 0 ) {
        page_num_ = page_num;
        line_num_ = line;

        Glib::signal_idle().connect( sigc::mem_fun(this, &DocManagerImpl::scroll_to_file_pos) );
    }
}

bool DocManagerImpl::scroll_to_file_pos() {
    if( page_num_ < 0 )
        return false;

    Gtk::Notebook::PageList::iterator it = pages().find(page_num_);
    if( it != pages().end() ) {
        set_current_page(page_num_);

        DocPageImpl* page = (DocPageImpl*)(get_current()->get_child());
        assert( page != 0 );

        Glib::RefPtr<gtksourceview::SourceBuffer> buffer = page->source_buffer();
        gtksourceview::SourceBuffer::iterator it = buffer->get_iter_at_line(line_num_);
        if( it != buffer->end() )
            buffer->place_cursor(it);
        page->view().scroll_to_iter(it, 0.0);

        page_num_ = -1;
    }
    return false;
}

void DocManagerImpl::open_file(const std::string& filepath, int line) {
    std::string abspath = filepath;
    ::ljcs_filepath_to_abspath(abspath);

    std::string filename = Glib::path_get_basename(filepath);

    Gtk::Notebook::PageList::iterator it = pages().begin();
    Gtk::Notebook::PageList::iterator end = pages().end();
    for( ; it!=end; ++it ) {
        DocPageImpl* page = (DocPageImpl*)(it->get_child());
        assert( page != 0 );

        if( page->filepath()==abspath ) {
            locate_page_line(it->get_page_num(), line);
            return;
        }
    }

	struct stat st;
	::memset(&st, 0, sizeof(st));

	if( ::stat(abspath.c_str(), &st)!=0 )
		return;

	std::string buf;
	try {
		buf.resize(st.st_size);

		std::ifstream ifs(abspath.c_str(), std::ios::in | std::ios::binary);
		ifs.read(&buf[0], buf.size());

	} catch(const std::exception&) {
		return;
	}

    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
	buffer->begin_not_undoable_action();
	const char* ps = &buf[0];
	const char* pe = ps + buf.size();
    buffer->set_text(ps, pe);
	buffer->end_not_undoable_action();
	buffer->set_modified(false);

    open_page(abspath, filename, buffer, line);
}

bool DocManagerImpl::open_page(const std::string filepath
        , const std::string& displaty_name
        , Glib::RefPtr<gtksourceview::SourceBuffer> buffer
        , int line)
{
    DocPageImpl* page = DocPageImpl::create(filepath, displaty_name, buffer);
    int n = append_page(*page, page->label());
    page->view().grab_focus();
    set_current_page(n);

    buffer->signal_modified_changed().connect(sigc::bind(sigc::mem_fun(this, &DocManagerImpl::on_doc_modified_changed), page));

    locate_page_line(n, line);

    return true;
}

bool DocManagerImpl::save_page(DocPageImpl& page) {
    if( !page.modified() )
        return true;

    Glib::ustring& filepath = page.filepath();
    if( filepath.empty() ) {
        Gtk::FileChooserDialog dlg("save...", Gtk::FILE_CHOOSER_ACTION_SAVE);
        dlg.add_button(Gtk::Stock::SAVE_AS, Gtk::RESPONSE_OK);
        dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        if( dlg.run()!=Gtk::RESPONSE_OK )
            return true;
        filepath = dlg.get_filename();
    }

    try {
        Glib::RefPtr<Glib::IOChannel> ofs = Glib::IOChannel::create_from_file(filepath, "w");
        ofs->write(page.buffer()->get_text());
        return true;
    } catch(Glib::FileError error) {
    }

    return false;
}

bool DocManagerImpl::close_page(DocPageImpl& page) {
    pages().erase( pages().find(page) );
    return true;
}

void DocManagerImpl::save_current_file() {
    Gtk::Widget* widget = get_current()->get_child();
    if( widget==0 )
        return;
    save_page( *((DocPageImpl*)widget) );
}

void DocManagerImpl::close_current_file() {
    Gtk::Widget* widget = get_current()->get_child();
    if( widget==0 )
        return;

    close_page( *((DocPageImpl*)widget) );
}

void DocManagerImpl::save_all_files() {
    PageList::iterator it = pages().begin();
    PageList::iterator end = pages().end();
    for( ; it!=end; ++it ) {
        Gtk::Widget* widget = get_current()->get_child();
        assert( widget != 0 );

        save_page( *(DocPageImpl*)(it->get_child()) );
    }
}

void DocManagerImpl::close_all_files() {
    pages().clear();
}

void DocManagerImpl::on_doc_modified_changed(DocPageImpl* page) {
    assert( page != 0 );
    Glib::ustring label = page->label().get_text();
    if( label.empty() )
        label = "noname";

    if( page->buffer()->get_modified() ) {
        if( *label.rbegin()!='*' ) {
            label += '*';
            page->label().set_text(label);
        }

    } else {
        if( *label.rbegin()=='*' ) {
            label.erase(label.size() - 1);
            page->label().set_text(label);
        }
    }
}

