// CmdLineCallbacks.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "CmdLineCallbacks.h"

// CmdGotoCallback

void CmdGotoCallback::on_active(void* tag) {
	gtksourceview::SourceView* view = (gtksourceview::SourceView*)tag;
	if( view==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = view->get_source_buffer();

	Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());
	if( it != buf->end() ) {
		char buf[64] = { '\0' };
		sprintf(buf, "%d", it.get_line() + 1);
		cmd_line_.entry().set_text(buf);
	}

	cmd_line_.entry().select_region(0, cmd_line_.entry().get_text_length());
}

void CmdGotoCallback::on_key_changed(void* tag) {
	gtksourceview::SourceView* view = (gtksourceview::SourceView*)tag;
	if( view==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = view->get_source_buffer();

	Glib::ustring text = cmd_line_.entry().get_text();

	int line_num = atoi(text.c_str());
	if( line_num > 0 ) {
		Gtk::TextIter it = buf->get_iter_at_line(line_num - 1);
		if( it != buf->end() ) {
			buf->place_cursor(it);
			view->scroll_to_iter(it, 0.25);
		}
	}
}

// CmdFindCallback

void CmdFindCallback::on_active(void* tag) {
	gtksourceview::SourceView* view = (gtksourceview::SourceView*)tag;
	if( view==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = view->get_source_buffer();

	Gtk::TextIter ps, pe;
	if( buf->get_selection_bounds(ps, pe) )
		cmd_line_.entry().set_text( buf->get_text(ps, pe) );

	cmd_line_.entry().select_region(0, cmd_line_.entry().get_text_length());
}

void CmdFindCallback::on_key_changed(void* tag) {
	gtksourceview::SourceView* view = (gtksourceview::SourceView*)tag;
	if( view==0 )
		return;

	Glib::RefPtr<gtksourceview::SourceBuffer> buf = view->get_source_buffer();

	gtksourceview::SourceIter it = buf->get_iter_at_mark(buf->get_insert());
	Gtk::TextIter end = buf->end();

	Glib::ustring text = cmd_line_.entry().get_text();

	Gtk::TextIter ps, pe;
	if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) ) {
		it = buf->begin();

		if( !it.forward_search(text, gtksourceview::SEARCH_TEXT_ONLY | gtksourceview::SEARCH_CASE_INSENSITIVE, ps, pe, end) )
			return;
	}

	buf->place_cursor(pe);
	buf->select_range(ps, pe);
	view->scroll_to_iter(ps, 0.25);
}

bool CmdFindCallback::on_key_press(GdkEventKey* event, void* tag) {
	gtksourceview::SourceView* view = (gtksourceview::SourceView*)tag;
	if( view==0 )
		return false;
	Glib::RefPtr<gtksourceview::SourceBuffer> buf = view->get_source_buffer();

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
			view->scroll_to_iter(ps, 0.3);
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
			view->scroll_to_iter(ps, 0.3);
		}
		return true;

	case GDK_Escape:
	case GDK_Return:
		cmd_line_.deactive();
		return true;
	}

	return false;
}

