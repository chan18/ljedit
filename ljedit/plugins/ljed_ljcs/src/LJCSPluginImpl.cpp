// LJCSPluginImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "LJCSPluginImpl.h"
#include "LJCSPluginUtils.h"

#include "SetupDialog.h"

#include "DocManager.h"
#include "LJEditor.h"

LJCSPluginImpl::LJCSPluginImpl(LJEditor& editor)
    : editor_(editor)
	, tip_(editor)
    , preview_(editor)
	, preview_page_(-1) {}

void LJCSPluginImpl::active_page(DocPage& page) {
	Gtk::TextView& view = page.view();

    assert( connections_map_.find(&page)==connections_map_.end() );
    TConnectionList& cons = connections_map_[&page];
    cons.push_back( view.signal_key_press_event().connect(		sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_key_press_event),		&page ), false ) );
    cons.push_back( view.signal_key_release_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_key_release_event),		&page )		   ) );
    cons.push_back( view.signal_button_release_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_button_release_event),	&page ), false ) );
    cons.push_back( view.signal_motion_notify_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_motion_notify_event),	&page )		   ) );
    cons.push_back( view.signal_focus_out_event().connect(		sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_focus_out_event),		&page )		   ) );

    if( !page.filepath().empty() ) {
        std::string filename = page.filepath();
        parse_thread_.add(filename);
    }
}

void LJCSPluginImpl::deactive_page(DocPage& page) {
    TConnectionListMap::iterator it = connections_map_.find(&page);
    if( it != connections_map_.end() ) {
        disconnect_all_connections(it->second);
        connections_map_.erase(it);
    }
}

void LJCSPluginImpl::show_hint(DocPage& page
    , Gtk::TextBuffer::iterator& it
    , Gtk::TextBuffer::iterator& end
    , char tag)
{
	kill_show_hint_timer();

    LJEditorDocIter ps(it);
    LJEditorDocIter pe(end);
	
    std::string filename = page.filepath();
    cpp::File* file = ParserEnviron::self().find_parsed(filename);
    if( file==0 )
        return;

    size_t line = (size_t)it.get_line() + 1;
    StrVector keys;
    if( !find_keys(keys, it, end, file) )
        return;

    MatchedSet mset;
    ::search_keys(keys, mset, *file, line);

    int view_x = 0;
    int view_y = 0;
    Gtk::TextView& view = page.view();
    view.get_window(Gtk::TEXT_WINDOW_TEXT)->get_origin(view_x, view_y);
    
    Gdk::Rectangle rect;
    view.get_iter_location(end, rect);

    int cursor_x = rect.get_x();
    int cursor_y = rect.get_y();
    view.buffer_to_window_coords(Gtk::TEXT_WINDOW_TEXT, cursor_x, cursor_y, cursor_x, cursor_y);

    tip_.show_tip(view_x + cursor_x, view_y + cursor_y + rect.get_height() + 2, mset.elems, tag);
}

void LJCSPluginImpl::auto_complete(DocPage& page) {
    cpp::Element* selected = tip_.get_selected();
    if( selected == 0 )
        return;

    Glib::RefPtr<Gtk::TextBuffer> buf = page.buffer();
    Glib::RefPtr<Gtk::TextMark> mark = buf->get_insert();
    Gtk::TextBuffer::iterator it = buf->get_iter_at_mark(mark);
    int finished = 0;
    while( it.backward_char() ) {
        char ch = (char)(it.get_char());
        if( ::isalnum(ch) || ch=='_' )
            finished += 1;
        else
            break;
    }
    
    buf->insert_at_cursor(selected->name.substr(finished));
    tip_.hide();
}

void LJCSPluginImpl::set_show_hint_timer(DocPage& page) {
	kill_show_hint_timer();

	++show_hint_tag_;
	show_hint_timer_ = Glib::signal_timeout().connect( sigc::bind(sigc::mem_fun(this, &LJCSPluginImpl::on_show_hint_timeout), &page, show_hint_tag_), 200 );
}

void LJCSPluginImpl::kill_show_hint_timer() {
	show_hint_timer_.disconnect();
}

bool LJCSPluginImpl::on_show_hint_timeout(DocPage* page, int tag) {
	assert( page != 0 );

	if( show_hint_tag_!=tag )
		return false;
	
	if( editor_.main_window().doc_manager().get_current_document()!=page )
		return false;

    Glib::RefPtr<Gtk::TextBuffer> buf = page->buffer();
    Gtk::TextBuffer::iterator it = buf->get_iter_at_mark(buf->get_insert());
    Gtk::TextBuffer::iterator end = it;
	show_hint(*page, it, end, 's');

	return false;
}

void LJCSPluginImpl::create(const char* plugin_filename) {
	plugin_path_ = Glib::path_get_dirname(plugin_filename);

    MainWindow& main_window = editor_.main_window();
    DocManager& dm = editor_.main_window().doc_manager();
    TConnectionList& cons = connections_map_[0];

	// menu
	action_group_ = Gtk::ActionGroup::create("LJCSActions");
    action_group_->add( Gtk::Action::create("LJCSSetup", Gtk::Stock::ABOUT,   "_ljcs",  "LJCS plugin setup"),	sigc::mem_fun(this, &LJCSPluginImpl::on_show_setup_dialog) );

	Glib::ustring ui_info = 
        "<ui>"
        "    <menubar name='MenuBar'>"
        "        <menu action='PluginsMenu'>"
        "            <menuitem action='LJCSSetup'/>"
        "        </menu>"
        "    </menubar>"
        "</ui>";

	main_window.ui_manager()->insert_action_group(action_group_);
	menu_id_ = main_window.ui_manager()->add_ui_from_string(ui_info);

    // auto complete
    cons.push_back( dm.signal_page_added().connect( sigc::mem_fun(this, &LJCSPluginImpl::on_doc_page_added) ) );
    cons.push_back( dm.signal_page_removed().connect( sigc::mem_fun(this, &LJCSPluginImpl::on_doc_page_removed) ) );

    // outline
    outline_.create();
    main_window.right_panel().append_page(outline_.get_widget(), "outline");

    cons.push_back(	outline_.signal_elem_actived.connect(	sigc::mem_fun(this, &LJCSPluginImpl::outline_on_elem_actived)	) );
    cons.push_back(	dm.signal_switch_page().connect(		sigc::mem_fun(this, &LJCSPluginImpl::outline_on_switch_page)	) );
    cons.push_back(	Glib::signal_timeout().connect(			sigc::mem_fun(this, &LJCSPluginImpl::outline_on_timeout), 500	) );

    // preview
    preview_.create();
    preview_page_ = main_window.bottom_panel().append_page(preview_.get_widget(), "preview");

	// init parser environ
	load_setup(plugin_path_);

	// start parse thread
	parse_thread_.run(); 
}

void LJCSPluginImpl::destroy() {
    MainWindow& main_window = editor_.main_window();
    DocManager& dm = main_window.doc_manager();

	// remove menu
	main_window.ui_manager()->remove_action_group(action_group_);
	main_window.ui_manager()->remove_ui(menu_id_);

	// stop parse thread
	parse_thread_.stop();

    // disconnect all signal connections
    TConnectionListMap::iterator it = connections_map_.begin();
    TConnectionListMap::iterator end = connections_map_.end();
    for( ; it!=end; ++it )
        disconnect_all_connections(it->second);
    connections_map_.clear();

    // auto complete

    // outline
    main_window.right_panel().remove_page(outline_.get_widget());

    // preview
    main_window.bottom_panel().remove_page(preview_.get_widget());
    preview_.destroy();
}

void LJCSPluginImpl::on_show_setup_dialog() {
	show_setup_dialog(editor_.main_window(), plugin_path_);
}

void LJCSPluginImpl::on_doc_page_added(Gtk::Widget* widget, guint page_num) {
    assert( widget != 0 );

    DocPage& page = editor_.main_window().doc_manager().child_to_page(*widget);
	active_page(page);
}

void LJCSPluginImpl::on_doc_page_removed(Gtk::Widget* widget, guint page_num) {
    assert( widget != 0 );

    DocPage& page = editor_.main_window().doc_manager().child_to_page(*widget);
    deactive_page(page);
}

bool LJCSPluginImpl::on_key_press_event(GdkEventKey* event, DocPage* page) {
    assert( page != 0 );

    if( !tip_.is_visible() )
        return false;

    if( event->state & (Gdk::CONTROL_MASK | Gdk::MOD1_MASK) ) {
        tip_.hide();
        return false;
    }

    switch( event->keyval ) {
    case GDK_Tab:
    //case GDK_Return:
		auto_complete(*page);
        return true;
    case GDK_Up:
        tip_.select_prev();
        return true;
    case GDK_Down:
        tip_.select_next();
        return true;
    case GDK_Escape:
        tip_.hide();
        return true;
    }

    return false;
}

bool LJCSPluginImpl::on_key_release_event(GdkEventKey* event, DocPage* page) {
    assert( page != 0 );

    if( event->state & (Gdk::CONTROL_MASK | Gdk::MOD1_MASK) ) {
        tip_.hide();
        return false;
    }

    switch( event->keyval ) {
    case GDK_Tab:
    case GDK_Return:
    case GDK_Up:
    case GDK_Down:
    case GDK_Escape:
	case GDK_Shift_L:
	case GDK_Shift_R:
        return false;
    }

	//printf("%d - %s\n", event->keyval, event->string);

    Glib::RefPtr<Gtk::TextBuffer> buf = page->buffer();
    Glib::RefPtr<Gtk::TextMark> mark = buf->get_insert();
    Gtk::TextBuffer::iterator it = buf->get_iter_at_mark(mark);
    Gtk::TextBuffer::iterator end = it;

    if( is_in_comment(it) )
        return false;

    switch( event->keyval ) {
    case '.':
        {
            show_hint(*page, it, end, 's');
        }
        break;
    case '(':
        {
            show_hint(*page, it, end, 'f');
        }
        break;
    case '<':
        {
            if( it.backward_chars(2) && it.get_char()!='<' ) {
                it.forward_chars(2);
                show_hint(*page, it, end, 'f');
            }
        }
        break;
    case ':':
        {
            if( it.backward_chars(2) && it.get_char()==':' ) {
                it.forward_chars(2);
                show_hint(*page, it, end, 's');
            }
        }
        break;
    case '>':
        {
            if( it.backward_chars(2) && it.get_char()=='-' ) {
                it.forward_chars(2);
                show_hint(*page, it, end, 's');
            }
        }
        break;
    default:
		if( (event->keyval <= 0x7f) && ::isalnum(event->keyval) || event->keyval=='_' )
			set_show_hint_timer(*page);
		else
			tip_.hide();
        break;
    }

    return true;
}

bool LJCSPluginImpl::on_button_release_event(GdkEventButton* event, DocPage* page) {
    assert( page != 0 );

    if( tip_.is_visible() )
        tip_.hide();

    cpp::File* file = ParserEnviron::self().find_parsed(page->filepath());
    if( file==0 )
        return false;

    // include test

    // tag test
    Glib::RefPtr<Gtk::TextBuffer> buf = page->buffer();
    Gtk::TextBuffer::iterator it = buf->get_iter_at_mark(buf->get_insert());
    char ch = (char)it.get_char();
    if( ::isalnum(ch) || ch=='_' ) {
        while( it.forward_word_end() ) {
            ch = it.get_char();
            if( ch=='_' )
                continue;
            break;
        }
    }
    Gtk::TextBuffer::iterator end = it;

    StrVector keys;
    if( !find_keys(keys, it, end, file) )
        return false;

    size_t line = (size_t)it.get_line() + 1;
    MatchedSet mset;
    ::search_keys(keys, mset, *file, line);

	// test CTRL state
	if( event->state & Gdk::CONTROL_MASK ) {
		// jump to
		cpp::Element* elem = find_best_matched_element(mset.elems);
		if( elem != 0 ) {
			DocManager& dm = editor_.main_window().doc_manager();
			dm.open_file(elem->file.filename, int(elem->sline - 1));
		}

	} else {
		// preview
		cpp::Elements elems;
		elems.resize(mset.elems.size());
		std::copy(mset.elems.begin(), mset.elems.end(), elems.begin());

		preview_.preview(elems, find_best_matched_index(elems));

		editor_.main_window().bottom_panel().set_current_page(preview_page_);
	}

    return false;
}

bool LJCSPluginImpl::on_motion_notify_event(GdkEventMotion* event, DocPage* page) {
    return true;
}

bool LJCSPluginImpl::on_focus_out_event(GdkEventFocus* event, DocPage* page) {
    tip_.hide();
    return true;
}

void LJCSPluginImpl::outline_update_page() {
    DocManager& dm = editor_.main_window().doc_manager();

    Gtk::Notebook::PageList::iterator it = dm.get_current();
    if( it!=dm.pages().end() ) {
        Gtk::Widget* widget = it->get_child();
        assert( widget != 0 );

        DocPage& page = dm.child_to_page(*widget);
        cpp::File* file = ParserEnviron::self().find_parsed(page.filepath());

		Glib::RefPtr<Gtk::TextBuffer> buf = page.buffer();
		Gtk::TextBuffer::iterator it = buf->get_iter_at_mark(buf->get_insert());
		outline_.set_file(file, it.get_line());

	} else {
		outline_.set_file(0, 0);
	}
}

void LJCSPluginImpl::outline_on_switch_page(GtkNotebookPage*, guint) {
    outline_update_page();
}

bool LJCSPluginImpl::outline_on_timeout() {
    outline_update_page();
    return true;
}

void LJCSPluginImpl::outline_on_elem_actived(const cpp::Element& elem) {
    DocManager& dm = editor_.main_window().doc_manager();
    dm.open_file(elem.file.filename, int(elem.sline - 1));
}

