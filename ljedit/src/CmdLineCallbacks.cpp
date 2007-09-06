// CmdLineCallbacks.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "CmdLineCallbacks.h"
#include "DocManagerImpl.h"


// BaseCmdCallback

bool BaseCmdCallback::base_active(void* tag) {
	tag_ = tag;

	current_page_ = doc_mgr_.get_current_doc_page();
	if( current_page_==0 )
		return false;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();
	last_pos_ = buf->get_iter_at_mark(buf->get_insert());
	return true;
}

bool BaseCmdCallback::base_on_key_press(GdkEventKey* event) {
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();
	Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());

	switch( event->keyval ) {

	case GDK_Escape:
		buf->place_cursor(last_pos_);
		current_page_->view().scroll_to_iter(last_pos_, 0.3);
		return false;

	case GDK_Return:
		doc_mgr_.pos_add(*current_page_, last_pos_.get_line(), last_pos_.get_line_offset());
		doc_mgr_.pos_add(*current_page_, it.get_line(), it.get_line_offset());
		return false;
	}

	return true;
}

// CmdGotoCallback

bool CmdGotoCallback::on_active(void* tag) {
	if( base_active(tag) ) {
		char buf[64] = { '\0' };
		sprintf(buf, "%d", last_pos_.get_line() + 1);
		cmd_line_.entry().set_text(buf);

		cmd_line_.label().set_text("goto:");
		cmd_line_.entry().select_region(0, cmd_line_.entry().get_text_length());

		return true;
	}

	return false;
}

bool CmdGotoCallback::on_key_changed() {
	assert( current_page_ != 0 );
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

	Glib::ustring text = cmd_line_.entry().get_text();

	int line_num = atoi(text.c_str());
	if( line_num > 0 ) {
		Gtk::TextIter it = buf->get_iter_at_line(line_num - 1);
		if( it != buf->end() ) {
			buf->place_cursor(it);
			current_page_->view().scroll_to_iter(it, 0.25);
		}
	}

	return true;
}

bool CmdGotoCallback::on_key_press(GdkEventKey* event) {
	switch( event->keyval ) {
	case GDK_Up:
	case GDK_Left:
		doc_mgr_.pos_back();
		return true;

	case GDK_Down:
	case GDK_Right:
		doc_mgr_.pos_forward();
		return true;
	}

	return base_on_key_press(event);
}


// CmdFindCallback

bool CmdFindCallback::on_active(void* tag) {
	if( base_active(tag) ) {
		assert( current_page_ != 0 );
		Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

		Gtk::TextIter ps, pe;
		if( buf->get_selection_bounds(ps, pe) )
			cmd_line_.entry().set_text( buf->get_text(ps, pe) );

		cmd_line_.label().set_text("find:");
		cmd_line_.entry().select_region(0, cmd_line_.entry().get_text_length());

		return true;
	}

	return false;
}

bool CmdFindCallback::on_key_changed() {
	assert( current_page_ != 0 );
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

	gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
	Gtk::TextIter end = buf->end();

	Glib::ustring text = cmd_line_.entry().get_text();

	Gtk::TextIter ps, pe;
	if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) ) {
		it = buf->begin();

		if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) )
			return true;
	}

	buf->place_cursor(pe);
	buf->select_range(ps, pe);
	current_page_->view().scroll_to_iter(ps, 0.25);

	return true;
}

bool CmdFindCallback::on_key_press(GdkEventKey* event) {
	assert( current_page_ != 0 );
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

	switch( event->keyval ) {
	case GDK_Up: {
			Glib::ustring text = cmd_line_.entry().get_text();

			gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
			Gtk::TextIter end = buf->begin();

			Gtk::TextIter ps, pe;
			if( !it.backward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) ) {
				it.forward_to_end();

				if( !it.backward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) )
					return true;
			}

			buf->select_range(ps, pe);
			current_page_->view().scroll_to_iter(ps, 0.3);
		}
		return true;

	case GDK_Down: {
			Glib::ustring text = cmd_line_.entry().get_text();

			gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
			Gtk::TextIter end = buf->end();

			if( !it.forward_char() )
				it = buf->begin();

			Gtk::TextIter ps, pe;
			if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) ) {
				it = buf->begin();

				if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) )
					return true;
			}

			buf->select_range(ps, pe);
			current_page_->view().scroll_to_iter(ps, 0.3);
		}
		return true;
	}

	return base_on_key_press(event);
}

