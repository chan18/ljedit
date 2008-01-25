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
	last_pos_ = buf->get_iter_at_mark(buf->get_insert()).get_offset();
	return true;
}

bool BaseCmdCallback::base_on_key_press(GdkEventKey* event) {
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();
	Gtk::TextIter last = buf->get_iter_at_offset(last_pos_);
	Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());

	switch( event->keyval ) {
	case GDK_Escape:
		buf->place_cursor(last);
		current_page_->view().scroll_to_iter(last, 0.3);
		cmd_line_.deactive();
		current_page_->view().grab_focus();
		return true;

	case GDK_Return:
		doc_mgr_.pos_add(*current_page_, last.get_line(), last.get_line_offset());
		doc_mgr_.pos_add(*current_page_, it.get_line(), it.get_line_offset());
		cmd_line_.deactive();
		current_page_->view().grab_focus();
		return true;
	}

	return false;
}

// CmdGotoCallback

bool CmdGotoCallback::on_active(void* tag) {
	if( base_active(tag) ) {
		assert( current_page_ != 0 );
		Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

		char text[64] = { '\0' };
#ifdef WIN32
		sprintf_s(text, "%d", buf->get_iter_at_mark(buf->get_insert()).get_line() + 1);
#else
		sprintf(text, "%d", buf->get_iter_at_mark(buf->get_insert()).get_line() + 1);
#endif
		cmd_line_.entry().set_text(text);

		//cmd_line_.desc().set_markup("<span background='red'>abccc</span><u>abccd</u>");
		cmd_line_.label().set_text("goto:");
		cmd_line_.entry().select_region(0, cmd_line_.entry().get_text_length());

		return true;
	}

	return false;
}

void CmdGotoCallback::on_key_changed() {
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

	if( base_on_key_press(event) )
		return true;

	return false;
}


// CmdFindCallback

namespace {

void do_find_next(DocPageImpl& page, const Glib::ustring& text) {
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page.buffer();

	gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
	Gtk::TextIter end = buf->end();

	if( !it.forward_char() )
		it = buf->begin();

	Gtk::TextIter ps, pe;
	if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) ) {
		it = buf->begin();

		if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) )
			return;
	}

	buf->select_range(ps, pe);
	page.view().scroll_to_iter(ps, 0.3);
}

void do_find_prev(DocPageImpl& page, const Glib::ustring& text) {
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = page.buffer();

	gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
	Gtk::TextIter end = buf->begin();

	Gtk::TextIter ps, pe;
	if( !it.backward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) ) {
		it.forward_to_end();

		if( !it.backward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) )
			return;
	}

	buf->select_range(ps, pe);
	page.view().scroll_to_iter(ps, 0.3);
}

void do_replace_all(Glib::RefPtr<gtksourceview::SourceBuffer>& buf, Glib::ustring& find_text, const Glib::ustring& replace_text) {
	gtksourceview::SourceIter it = buf->begin();

	buf->begin_user_action();
	{
		Gtk::TextIter ps, pe;
		while( it.forward_search(find_text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, buf->end()) ) {
			buf->place_cursor( buf->erase(ps, pe) );
			buf->insert_at_cursor(replace_text);
			it = buf->get_iter_at_mark(buf->get_insert());
		}
	}
	buf->end_user_action();
}

} // anonymous namespace

bool CmdFindCallback::on_active(void* tag) {
	if( base_active(tag) ) {
		assert( current_page_ != 0 );
		Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

		Gtk::TextIter ps, pe;
		if( buf->get_selection_bounds(ps, pe) && ps!=pe )
			last_find_text_ = buf->get_text(ps, pe);

		cmd_line_.label().set_text("find:");
		cmd_line_.entry().set_text( last_find_text_ );
		cmd_line_.entry().select_region(0, (int)last_find_text_.size());
		//printf("find : %s : %d\n", last_find_text_.c_str(), last_find_text_.size());

		return true;
	}

	return false;
}

void CmdFindCallback::on_key_changed() {
	assert( current_page_ );

	last_find_text_ = cmd_line_.entry().get_text();
	do_find_next(*current_page_, last_find_text_);
}

bool CmdFindCallback::on_key_press(GdkEventKey* event) {
	assert( current_page_ );

	switch( event->keyval ) {
	case GDK_Up:
		do_find_prev(*current_page_, last_find_text_);
		return true;

	case GDK_Down:
		do_find_next(*current_page_, last_find_text_);
		return true;
	}

	return base_on_key_press(event);
}


// CmdReplaceCallback

bool CmdReplaceCallback::on_active(void* tag) {
	if( base_active(tag) ) {
		assert( current_page_ != 0 );
		Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

		size_t sel_pos = 0;
		Gtk::TextIter ps, pe;
		if( buf->get_selection_bounds(ps, pe) )
		{
			last_find_text_ = buf->get_text(ps, pe);
			sel_pos = last_find_text_.size() + 1;
		}
		else
		{
			last_find_text_.clear();
		}

		Glib::ustring text;
		text = last_find_text_;
		if( text.empty() )
			text = "<find>";
		text += "/<replace>/[all]";

		cmd_line_.label().set_text("replace:");
		cmd_line_.entry().set_text( text );
		cmd_line_.entry().select_region((int)sel_pos, (int)text.size());
		//printf("replace : %s : (%d, %d)\n", text.c_str(), sel_pos, text.size());

		return true;
	}

	return false;
}

void CmdReplaceCallback::on_key_changed() {
	assert( current_page_ );
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

	gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
	Gtk::TextIter end = buf->end();

	Glib::ustring text = cmd_line_.entry().get_text();
	size_t pos = text.find('/');
	if( pos != text.npos )
		text.erase(pos);

	if( text==last_find_text_ )
		return;

	last_find_text_ = text;
	do_find_next(*current_page_, last_find_text_);
}

bool CmdReplaceCallback::on_key_press(GdkEventKey* event) {
	assert( current_page_ );

	if( event->keyval==GDK_Return ) {
		Glib::ustring find_text, replace_text;
		bool replace_all_sign = false;

		Glib::ustring text = cmd_line_.entry().get_text();
		size_t find_pos = text.find('/');
		if( find_pos == text.npos )
		{
			find_pos = text.size();
			text += "/<replace>/[all]";
			cmd_line_.entry().set_text(text);
			cmd_line_.entry().select_region((int)find_pos, (int)text.size());
			return true;
		}
		find_text.assign(text, 0, find_pos);
		++find_pos;

		size_t replace_pos = text.find('/', find_pos);
		if( replace_pos == text.npos ) {
			replace_text.assign(text, find_pos, text.size() - find_pos);

		} else {
			replace_text.assign(text, find_pos, replace_pos - find_pos);
			++replace_pos;

			if( text.compare(replace_pos, text.size() - replace_pos, "all", 3)==0 )
				replace_all_sign = true;
		}

		Glib::RefPtr<gtksourceview::SourceBuffer> buf = current_page_->buffer();

		if( replace_all_sign ) {
			do_replace_all(buf, find_text, replace_text);

			cmd_line_.deactive();
			current_page_->view().grab_focus();

		} else {
			Gtk::TextIter start, end;
			buf->get_selection_bounds(start, end);
			Glib::ustring sel = buf->get_text(start, end);
			if( sel!=last_find_text_ )
			{
				cmd_line_.deactive();
				current_page_->view().grab_focus();
				return true;
			}

			buf->begin_user_action();
			{
				buf->erase_selection();
				buf->insert_at_cursor(replace_text);
			}
			buf->end_user_action();
			do_find_next(*current_page_, last_find_text_);
		}
		return true;
	}

	return CmdFindCallback::on_key_press(event);
}

