// LJCS.cpp
// 

#include "LJCS.h"

#include "PreviewPage.h"
#include "OutlinePage.h"
#include "Tips.h"

#include <gdk/gdkkeysyms.h>

GRegex* re_include = g_regex_new("^[ \t]*#[ \t]*include[ \t]*(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
GRegex* re_include_tip = g_regex_new("([\"<])(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
GRegex* re_include_info = g_regex_new("([\"<])([^\">]*)[\">].*", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);


LJCS::LJCS() : app(0) {}

LJCS::~LJCS() { destroy(); }

gboolean ljcs_update_timeout(LJCS* self) {
	preview_page_update(self->preview_page);
	outline_page_update(self->outline_page);

	return TRUE;
}

bool LJCS::create(Puss* _app) {
	app = _app;

	icons.create(app);

	preview_page = preview_page_create(app, &env);
	outline_page = outline_page_create(app, &env, &icons);
	tips = tips_create(app, &env, &icons);

	GtkNotebook* doc_panel = puss_get_doc_panel(app);
	g_signal_connect(doc_panel, "page-added",   G_CALLBACK(&LJCS::on_doc_page_added),   this);
	g_signal_connect(doc_panel, "page-removed", G_CALLBACK(&LJCS::on_doc_page_removed), this);

	parse_thread.run(&env);

	g_timeout_add(500, (GSourceFunc)&ljcs_update_timeout, this);

	return true;
}

void LJCS::destroy() {
	parse_thread.stop();
	
	tips_destroy(tips);
	outline_page_destroy(outline_page);
	preview_page_destroy(preview_page);

	icons.destroy();

	app = 0;
}

void LJCS::open_include_file(const gchar* filename, gboolean system_header, const gchar* owner_path) {
	gboolean succeed = FALSE;

	if( owner_path && !system_header ) {
		gchar* dirpath = g_path_get_dirname(owner_path);
		gchar* filepath = g_build_filename(dirpath, filename, NULL);
		succeed = app->doc_open(filepath, -1, -1);
		g_free(dirpath);
		g_free(filepath);
	}

	StrVector paths = env.get_include_paths();
	StrVector::iterator it = paths.begin();
	StrVector::iterator end = paths.end();
	for( ; !succeed && it!=end; ++it ) {
		gchar* filepath = g_build_filename(it->c_str(), filename, NULL);
		succeed = app->doc_open(filepath, -1, -1);
		g_free(filepath);
	}
}

void LJCS::do_button_release_event(GtkWidget* view, GtkTextBuffer* buf, GdkEventButton* event, cpp::File* file) {
    // tag test
	GtkTextIter it;
	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
	if( is_in_comment(&it) )
		return;

    // include test
	if( event->state & GDK_CONTROL_MASK ) {
		GtkTextIter ps = it;
		GtkTextIter pe = it;
		gtk_text_iter_set_line_offset(&ps, 0);
		gtk_text_iter_forward_to_line_end(&pe);

		gchar* line = gtk_text_iter_get_text(&ps, &pe);
		GMatchInfo* inc_info = 0;
		gboolean is_include_line = g_regex_match(re_include, line, (GRegexMatchFlags)0, &inc_info);

		if( is_include_line ) {
			gchar* inc_info_text = g_match_info_fetch(inc_info, 1);
			GMatchInfo* info = 0;
			if( g_regex_match(re_include_info, inc_info_text, (GRegexMatchFlags)0, &info) ) {
				gchar* sign = g_match_info_fetch(info, 1);
				gchar* filename = g_match_info_fetch(info, 2);
				if( file ) {
					open_include_file(filename, *sign=='<', file->filename.c_str());
				} else {
					GString* filepath = app->doc_get_url(buf);
					if( filepath )
						open_include_file(filename, *sign=='<', filepath->str);
				}
				g_free(sign);
				g_free(filename);
			}
			g_free(inc_info_text);
			g_match_info_free(info);
		}
		g_match_info_free(inc_info);
		g_free(line);

		if( is_include_line )
			return;
	}

	if( !file )
		return;

    char ch = (char)gtk_text_iter_get_char(&it);
    if( isalnum(ch)==0 && ch!='_' )
		return;

	// find key end position
	while( gtk_text_iter_forward_char(&it) ) {
        ch = (char)gtk_text_iter_get_char(&it);
        if( ch > 0 && (::isalnum(ch) || ch=='_') )
            continue;
        break;
    }

	// find keys
	GtkTextIter end = it;
	size_t line = (size_t)gtk_text_iter_get_line(&it) + 1;

	// test CTRL state
	if( event->state & GDK_CONTROL_MASK ) {
		// jump to
		StrVector keys;
		if( !find_keys(keys, &it, &end, file, FALSE) )
			return;

		MatchedSet mset(env);
		if( env.stree().reader_try_lock() ) {
			::search_keys(keys, mset, env.stree().ref(), file, line);
			env.stree().reader_unlock();

			cpp::Element* elem = find_best_matched_element(mset.elems());
			if( elem )
				app->doc_open(elem->file.filename.c_str(), int(elem->sline - 1), -1);
		}


	} else {
		// preview
		LJEditorDocIter ps(&it);
		LJEditorDocIter pe(&end);
		std::string key;
		if( !find_key(key, ps, pe, false) )
			return;

		gchar* key_text = gtk_text_iter_get_text(&it, &end);
		preview_page_preview(preview_page, key.c_str(), key_text, *file, line);
		g_free(key_text);
	}
}

gboolean LJCS::on_key_press_event(GtkWidget* view, GdkEventKey* event, LJCS* self) {
    if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) ) {
        //tip_.hide_all_tip();
        return FALSE;
    }

	switch( event->keyval ) {
	case GDK_Tab:
		//if( tip_.list_window().is_visible() || tip_.include_window().is_visible() ) {
		//	auto_complete(*page);
		//	return TRUE;
		//}
		break;

	case GDK_Return:
		//tip_.hide_all_tip();
		break;

	case GDK_Up:
		//if( tip_.list_window().is_visible() || tip_.include_window().is_visible() ) {
		//	tip_.select_prev();
		//	return TRUE;
		//}

		//if( tip_.decl_window().is_visible() )
		//	tip_.decl_window().hide();
		break;

	case GDK_Down:
		//if( tip_.list_window().is_visible() || tip_.include_window().is_visible() ) {
		//	tip_.select_next();
		//	return TRUE;
		//}

		//if( tip_.decl_window().is_visible() )
		//	tip_.decl_window().hide();
		break;

	case GDK_Escape:
		//tip_.hide_all_tip();
		return TRUE;
	}

	return FALSE;
}

gboolean LJCS::on_key_release_event(GtkWidget* view, GdkEventKey* event, LJCS* self) {
    if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) ) {
        //tip_.hide_all_tip();
        return FALSE;
    }

    switch( event->keyval ) {
    case GDK_Tab:
    case GDK_Return:
    case GDK_Up:
    case GDK_Down:
    case GDK_Escape:
	case GDK_Shift_L:
	case GDK_Shift_R:
        return FALSE;
    }

	//printf("%d - %s\n", event->keyval, event->string);

	/*
    Glib::RefPtr<Gtk::TextBuffer> buf = page->buffer();
    Gtk::TextIter it = buf->get_iter_at_mark(buf->get_insert());

    if( is_in_comment(it) )
        return false;
 
	// include tip
	{
		Gtk::TextIter ps = it;
		Gtk::TextIter pe = it;
		ps.set_line_offset(0);
		pe.forward_to_line_end();
		Glib::ustring line = ps.get_text(pe);

		GMatchInfo* inc_info = 0;
		bool is_start = false;
		bool is_include_line = g_regex_match(re_include, line.c_str(), (GRegexMatchFlags)0, &inc_info);
		if( is_include_line ) {
			gchar* inc_info_text = g_match_info_fetch(inc_info, 1);
			GMatchInfo* info = 0;
			if( g_regex_match(re_include_tip, inc_info_text, (GRegexMatchFlags)0, &info) ) {
				gchar* sign = g_match_info_fetch(info, 1);
				gchar* filename = g_match_info_fetch(info, 2);
				if( filename[0]=='\0' )
					is_start = true;
				show_include_hint(filename, *sign=='<', *page);
				g_free(sign);
				g_free(filename);
			}
			g_free(inc_info_text);
			g_match_info_free(info);
		}
		g_match_info_free(inc_info);

		if( is_include_line ) {
			switch( event->keyval ) {
			case '"':
				if( is_start )
					break;
			case '>':
				tip_.include_window().hide();
				break;
			}

			return false;
		}
	}

    Gtk::TextIter end = it;

    switch( event->keyval ) {
    case '.':
        {
            show_hint(*page, it, end, 's');
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
    case '(':
        {
            show_hint(*page, it, end, 'f');
        }
        break;
	case ')':
		{
			if( tip_.decl_window().is_visible() ) {
				tip_.decl_window().hide();
			}
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
    case '>':
        {
            if( it.backward_chars(2) && it.get_char()=='-' ) {
                it.forward_chars(2);
                show_hint(*page, it, end, 's');

			} else if( tip_.decl_window().is_visible() ) {
				tip_.decl_window().hide();
			}
        }
        break;
    default:
		if( (event->keyval <= 0x7f) && ::isalnum(event->keyval) || event->keyval=='_' ) {
			if( tip_.list_window().is_visible() ) {
				locate_sub_hint(*page);

			} else {
				set_show_hint_timer(*page);
			}
		} else {
			tip_.list_window().hide();
		}
        break;
    }
	*/

	return TRUE;
}

gboolean LJCS::on_button_release_event(GtkWidget* view, GdkEventButton* event, LJCS* self) {
    //tip_.hide_all_tip();

	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GString* url = self->app->doc_get_url(buf);
	if( url ) {
		cpp::File* file = self->env.find_parsed(std::string(url->str, url->len));
		self->do_button_release_event(view, buf, event, file);
		if( file )
			self->env.file_decref(file);
	}

    return FALSE;
}

gboolean LJCS::on_focus_out_event(GtkWidget* view, GdkEventFocus* event, LJCS* self) {
    //tip_.hide_all_tip();
	return FALSE;
}

gboolean LJCS::on_scroll_event(GtkWidget* view, GdkEventScroll* event, LJCS* self) {
    //tip_.hide_all_tip();
	return FALSE;
}

void LJCS::on_modified_changed(GtkTextBuffer* buf, LJCS* self) {
	GString* url = self->app->doc_get_url(buf);
	if( url )
		self->parse_thread.add(std::string(url->str, url->len));
}

void LJCS::on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self) {
	GtkTextView* view = self->app->doc_get_view_from_page(page);
	if( !view )
		return;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	g_signal_connect(view, "key-press-event", G_CALLBACK(&LJCS::on_key_press_event), self);
	g_signal_connect(view, "key-release-event", G_CALLBACK(&LJCS::on_key_release_event), self);

	g_signal_connect(view, "button-release-event", G_CALLBACK(&LJCS::on_button_release_event), self);
	g_signal_connect(view, "focus-out-event", G_CALLBACK(&LJCS::on_focus_out_event), self);
	g_signal_connect(view, "scroll-event", G_CALLBACK(&LJCS::on_scroll_event), self);

	g_signal_connect(buf, "modified-changed", G_CALLBACK(&LJCS::on_modified_changed), self);
	
	GString* url = self->app->doc_get_url(buf);
	if( url )
		self->parse_thread.add(std::string(url->str, url->len));
}

void LJCS::on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self) {
}

