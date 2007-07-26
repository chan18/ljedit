// DocManagerImpl.cpp
// 

#include "DocManagerImpl.h"

#include <fstream>

#include "LanguageManager.h"
#include "dir_utils.h"

PageImpl* PageImpl::create(const std::string& filepath
        , const std::string& display_name
        , Glib::RefPtr<gtksourceview::SourceBuffer> buffer)
{
    Gtk::Label* label = Gtk::manage(new Gtk::Label(display_name));

    gtksourceview::SourceView* view = Gtk::manage(new gtksourceview::SourceView(buffer));
    view->set_wrap_mode(Gtk::WRAP_NONE);
    view->set_highlight_current_line();
        
    PageImpl* page = Gtk::manage(new PageImpl(*label, *view));
    page->filepath_ = filepath;
    page->add(*view);
    page->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    page->show_all();

    return page;
}

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

        PageImpl* page = (PageImpl*)(get_current()->get_child());
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
        PageImpl* page = (PageImpl*)(it->get_child());
        assert( page != 0 );

        if( page->filepath()==abspath ) {
            locate_page_line(it->get_page_num(), line);
            return;
        }
    }

    Glib::ustring text;
    try {
        Glib::RefPtr<Glib::IOChannel> ifs = Glib::IOChannel::create_from_file(abspath.c_str(), "r");
        ifs->read_to_end(text);

    } catch(Glib::FileError error) {
        return;
    }

    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
    buffer->set_text(text);
    open_page(abspath, filename, buffer, line);
}

bool DocManagerImpl::open_page(const std::string filepath
        , const std::string& displaty_name
        , Glib::RefPtr<gtksourceview::SourceBuffer> buffer
        , int line)
{
    PageImpl* page = PageImpl::create(filepath, displaty_name, buffer);
    int n = append_page(*page, page->label());
    page->view().grab_focus();
    set_current_page(n);

    buffer->signal_modified_changed().connect(sigc::bind(sigc::mem_fun(this, &DocManagerImpl::on_doc_modified_changed), page));

    locate_page_line(n, line);

    return true;
}

bool DocManagerImpl::save_page(PageImpl& page) {
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

bool DocManagerImpl::close_page(PageImpl& page) {
    pages().erase( pages().find(page) );
    return true;
}

void DocManagerImpl::save_current_file() {
    Gtk::Widget* widget = get_current()->get_child();
    if( widget==0 )
        return;
    save_page( *((PageImpl*)widget) );
}

void DocManagerImpl::close_current_file() {
    Gtk::Widget* widget = get_current()->get_child();
    if( widget==0 )
        return;

    close_page( *((PageImpl*)widget) );
}

void DocManagerImpl::save_all_files() {
    PageList::iterator it = pages().begin();
    PageList::iterator end = pages().end();
    for( ; it!=end; ++it ) {
        Gtk::Widget* widget = get_current()->get_child();
        assert( widget != 0 );

        save_page( *(PageImpl*)(it->get_child()) );
    }
}

void DocManagerImpl::close_all_files() {
    pages().clear();
}

Gtk::TextView* DocManagerImpl::create_source_view() {
    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = create_cppfile_buffer();
    gtksourceview::SourceView* view = new gtksourceview::SourceView(buffer);
    if( view != 0 ) {
        view->set_wrap_mode(Gtk::WRAP_NONE);
        view->set_highlight_current_line();
    }
    return view;
}

void DocManagerImpl::destroy_source_view(Gtk::TextView* view) {
    delete view;
}

void DocManagerImpl::on_doc_modified_changed(PageImpl* page) {
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

