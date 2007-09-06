// DocManagerImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "DocManagerImpl.h"

#include "LJEditorUtilsImpl.h"
#include "dir_utils.h"

DocManagerImpl::DocManagerImpl() : locate_page_num_(-1) {
	pos_pool_init();

	sig_page_close_ = signal_page_removed().connect( sigc::mem_fun(this, &DocManagerImpl::on_page_removed) );
}

DocManagerImpl::~DocManagerImpl() {
	sig_page_close_.disconnect();
    close_all_files();
}

void DocManagerImpl::create_new_file() {
    static std::string noname = "noname";

    open_page("", noname);
}

void DocManagerImpl::locate_page_line(int page_num, int line, int line_offset, bool record_pos) {
    if( locate_page_num_ < 0 ) {
        locate_page_num_ = page_num;
        locate_line_num_ = line;
		locate_line_offset_ = line_offset;
		locate_record_pos_  = record_pos;

        Glib::signal_idle().connect( sigc::mem_fun(this, &DocManagerImpl::scroll_to_file_pos) );
    }
}

bool DocManagerImpl::scroll_to_file_pos() {
    if( locate_page_num_ < 0 )
        return false;

	DocPageImpl* current_page = 0;
	int          current_line = 0;
	int          current_lpos = 0;
	if( get_current()!=pages().end() ) {
		current_page = (DocPageImpl*)get_current()->get_child();
		
        Glib::RefPtr<gtksourceview::SourceBuffer> buffer = current_page->source_buffer();
		Gtk::TextIter it = buffer->get_iter_at_mark(buffer->get_insert());
		current_line = it.get_line();

		if( locate_page_num_==get_current_page() && locate_line_num_==current_line ) {
			locate_page_num_ = -1;
			return false;
		}

		current_lpos = it.get_line_offset();
	}

    Gtk::Notebook::PageList::iterator it = pages().find(locate_page_num_);
    if( it != pages().end() ) {
		if( locate_record_pos_ && current_page!=0 )
			pos_add(*current_page, current_line, current_lpos);

        set_current_page(locate_page_num_);

        DocPageImpl* page = (DocPageImpl*)get_current()->get_child();
        assert( page != 0 );
        page->view().grab_focus();

        Glib::RefPtr<gtksourceview::SourceBuffer> buffer = page->source_buffer();
        gtksourceview::SourceBuffer::iterator it = buffer->get_iter_at_line_offset(locate_line_num_, locate_line_offset_);
		if( it != buffer->end() )
            buffer->place_cursor(it);
        page->view().scroll_to_iter(it, 0.25);

        locate_page_num_ = -1;

		if( locate_record_pos_ )
			pos_add(*page, locate_line_num_, locate_line_offset_);
    }
    return false;
}

void DocManagerImpl::open_file(const std::string& filepath, int line, int line_offset) {
    std::string abspath = filepath;
    ::ljcs_filepath_to_abspath(abspath);

	if( do_locate_file(abspath, line) )
		return;

    std::string filename = Glib::path_get_basename(filepath);

	Glib::ustring ubuf;
	if( !LJEditorUtilsImpl::self().load_file(ubuf, abspath) )
		return;

    open_page(abspath, filename, &ubuf, line, line_offset);
}

bool DocManagerImpl::do_locate_file(const std::string& abspath, int line, int line_offset) {
    Gtk::Notebook::PageList::iterator it = pages().begin();
    Gtk::Notebook::PageList::iterator end = pages().end();
    for( ; it!=end; ++it ) {
        DocPageImpl* page = (DocPageImpl*)it->get_child();
        assert( page != 0 );

        if( page->filepath()==abspath ) {
            locate_page_line(it->get_page_num(), line, line_offset);
            return true;
        }
    }

	return false;
}

bool DocManagerImpl::locate_file(const std::string& filepath, int line, int line_offset) {
    std::string abspath = filepath;
    ::ljcs_filepath_to_abspath(abspath);

	return do_locate_file(abspath, line, line_offset);
}

bool DocManagerImpl::on_page_label_button_press(GdkEventButton* event, DocPageImpl* page) {
	if( event->button==2 ) {
		// mouse mid button
		close_page(*page);
	}
	return false;
}

bool DocManagerImpl::open_page(const std::string filepath
        , const std::string& displaty_name
        , const Glib::ustring* text
        , int line
		, int line_offset)
{
    DocPageImpl* page = DocPageImpl::create(filepath, displaty_name);
    if( page==0 )
    	return false;

	if( text != 0 ) {
	    Glib::RefPtr<gtksourceview::SourceBuffer> buffer = page->source_buffer();
		buffer->begin_not_undoable_action();
		buffer->set_text(*text);
		buffer->place_cursor(buffer->get_iter_at_line(line));
		buffer->end_not_undoable_action();
		buffer->set_modified(false);
		buffer->signal_modified_changed().connect( sigc::bind(sigc::mem_fun(this, &DocManagerImpl::on_doc_modified_changed), page) );
	}
	
	page->label_event_box().signal_button_release_event().connect( sigc::bind(sigc::mem_fun(this, &DocManagerImpl::on_page_label_button_press), page) );

    int n = append_page(*page, page->label_event_box());

    locate_page_line(n, line, line_offset);

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
        page.buffer()->set_modified(false);
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

void DocManagerImpl::on_page_removed(Gtk::Widget* widget, guint page_num) {
	DocPage* page = (DocPageImpl*)widget;

	for( PosNode* node = pos_first_; node != 0; node = node->next ) {
		if( node->page == page ) {
			if( pos_first_==node ) {
				pos_first_ = node->next;
				if( pos_first_!=0 )
					pos_first_->prev = 0;
			}

			if( pos_cur_==node)
				pos_cur_ = node->next;

			if( node->prev != 0 )
				node->prev->next = node->next;
			if( node->next != 0 )
				node->next->prev = node->prev;
			pos_nodes_.push_back(node);
		}
	}
}

void DocManagerImpl::pos_pool_init() {
	size_t count = sizeof(_pos_pool_) / sizeof(PosNode);
	assert( count > 2 );
	for( size_t i=0; i<count; ++i )
		pos_nodes_.push_back(&_pos_pool_[i]);
}

void DocManagerImpl::pos_add(DocPageImpl& page, int line, int line_offset) {
	PosNode* node = 0;

	if( pos_cur_ != 0 ) {
		// if last==current, not record
		if( pos_cur_->page==&page && pos_cur_->line==line ) {
			pos_cur_->lpos = line_offset;
			return;
		}

		// remove forwards
		for( node = pos_cur_->next; node!=0; node = node->next )
			pos_nodes_.push_back(node);

		if( pos_nodes_.empty() ) {
			assert( pos_first_!=0 && pos_first_->next!=0 );
			node = pos_first_;
			pos_first_ = node->next;
			pos_first_->prev = 0;

		} else {
			node = pos_nodes_.back();
			pos_nodes_.pop_back();
		}

		pos_cur_->next = node;

	} else {
		for( node = pos_first_; node!=0; node = node->next )
			pos_nodes_.push_back(node);

		assert( !pos_nodes_.empty() );
		pos_first_ = pos_nodes_.back();
		pos_nodes_.pop_back();
		node = pos_first_;
	}

	node->next = 0;
	node->prev = pos_cur_;
	node->page = &page;
	node->line = line;
	node->lpos = line_offset;

	pos_cur_ = node;
}

void DocManagerImpl::pos_forward() {
	if( pos_cur_==0 || pos_cur_->next==0 )
		return;

	pos_cur_ = pos_cur_->next;
	locate_page_line(page_num(*pos_cur_->page), pos_cur_->line, pos_cur_->lpos, false);
}

void DocManagerImpl::pos_back() {
	if( pos_cur_==0 )
		return;

	if( pos_cur_->next==0 ) {
		// if last and different position, record current position
		if( get_current()!=pages().end() ) {
			DocPageImpl* current_page = (DocPageImpl*)get_current()->get_child();
			
			Glib::RefPtr<gtksourceview::SourceBuffer> buffer = current_page->source_buffer();
			Gtk::TextIter it = buffer->get_iter_at_mark(buffer->get_insert());

			pos_add(*current_page, it.get_line(), it.get_line_offset());
		}
	}

	if( pos_cur_->prev==0 )
		return;

	pos_cur_ = pos_cur_->prev;
	locate_page_line(page_num(*pos_cur_->page), pos_cur_->line, pos_cur_->lpos, false);
}

