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
		tips_hide_all(self->tips);
        return FALSE;
    }

	switch( event->keyval ) {
	case GDK_Tab:
		if( tips_list_is_visible(self->tips) || tips_include_is_visible(self->tips) ) {
			// auto_complete(*page);
			return TRUE;
		}
		break;

	case GDK_Return:
		tips_hide_all(self->tips);
		break;

	case GDK_Up:
		if( tips_list_is_visible(self->tips) || tips_include_is_visible(self->tips) ) {
			tips_select_prev(self->tips);
			return TRUE;
		}

		if( tips_decl_is_visible(self->tips) )
			tips_decl_tip_hide(self->tips);
		break;

	case GDK_Down:
		if( tips_list_is_visible(self->tips) || tips_include_is_visible(self->tips) ) {
			tips_select_next(self->tips);
			return TRUE;
		}

		if( tips_decl_is_visible(self->tips) )
			tips_decl_tip_hide(self->tips);
		break;

	case GDK_Escape:
		tips_hide_all(self->tips);
		return TRUE;
	}

	return FALSE;
}

void LJCS::show_include_hint(const gchar* filename, gboolean system_header, GtkTextView* view) {
/*
	std::string key;
	std::set<std::string> files;
	std::string path;

	if( !filename.empty() ) {
		char last = filename[filename.size() -1];
		if( last=='/' || last=='\\' ) {
			;
		} else {
			size_t pos = filename.find_last_of("/\\");
			if( pos!=filename.npos )
				key = filename.substr(pos+1);
			else
				key = filename;
		}
	}

	if( system_header ) {
		StrVector paths = LJCSEnv::self().get_include_paths();
		StrVector::iterator it = paths.begin();
		StrVector::iterator end = paths.end();
		for( ; it!=end; ++it ) {
			try {
				if( filename.empty() ) {
					path = *it;
				} else {
					path = Glib::build_filename(*it, filename);
					path = Glib::path_get_dirname(path);
				}

				Glib::Dir dir(path);
				for( Glib::DirIterator it = dir.begin(); it!=dir.end(); ++it) {
					path = *it;
					if( key.empty() || path.find(key)==0 )
						files.insert(path);
				}
			} catch( const Glib::Exception& ) {
			}
		}

	} else {
		try {
			path = Glib::path_get_dirname(page.filepath());
			if( !filename.empty() ) {
				path = Glib::build_filename(path, filename);
				path = Glib::path_get_dirname(path);
			}

			Glib::Dir dir(path);
			for( Glib::DirIterator it = dir.begin(); it!=dir.end(); ++it) {
				path = *it;
				if( key.empty() || path.find(key)==0 )
					files.insert(path);
			}

		} catch( const Glib::Exception& ) {
		}
	}

	if( !files.empty() ) {
		int view_x = 0;
		int view_y = 0;
		Gtk::TextView& view = page.view();
		view.get_window(Gtk::TEXT_WINDOW_TEXT)->get_origin(view_x, view_y);
	    
		Gdk::Rectangle rect;
		view.get_iter_location(page.buffer()->get_iter_at_mark(page.buffer()->get_insert()), rect);

		int cursor_x = rect.get_x();
		int cursor_y = rect.get_y();
		view.buffer_to_window_coords(Gtk::TEXT_WINDOW_TEXT, cursor_x, cursor_y, cursor_x, cursor_y);

		int x = view_x + cursor_x;
		int y = view_y + cursor_y + rect.get_height() + 2;

		tip_.show_include_tip(x, y, files);

	} else {
		tip_.include_window().hide();
	}
*/
}

gboolean LJCS::on_key_release_event(GtkWidget* view, GdkEventKey* event, LJCS* self) {
    if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) ) {
        tips_hide_all(self->tips);
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

	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	if( is_in_comment(&iter) )
		return FALSE;

	// include tip
	{
		GtkTextIter ps = iter;
		GtkTextIter pe = iter;
		gtk_text_iter_set_line_offset(&ps, 0);
		if( !gtk_text_iter_forward_to_line_end(&pe) )
			return FALSE;

		gchar* line = gtk_text_iter_get_text(&ps, &pe);
		GMatchInfo* inc_info = 0;
		gboolean is_start = FALSE;
		gboolean is_include_line = g_regex_match(re_include, line, (GRegexMatchFlags)0, &inc_info);
		if( is_include_line ) {
			gchar* inc_info_text = g_match_info_fetch(inc_info, 1);
			GMatchInfo* info = 0;
			if( g_regex_match(re_include_tip, inc_info_text, (GRegexMatchFlags)0, &info) ) {
				gchar* sign = g_match_info_fetch(info, 1);
				gchar* filename = g_match_info_fetch(info, 2);
				if( filename[0]=='\0' )
					is_start = TRUE;

				self->show_include_hint(filename, *sign=='<', GTK_TEXT_VIEW(view));

				g_free(sign);
				g_free(filename);
			}
			g_free(inc_info_text);
			g_match_info_free(info);
		}
		g_match_info_free(inc_info);
		g_free(line);

		if( is_include_line ) {
			switch( event->keyval ) {
			case '"':
				if( is_start )
					break;
			case '>':
				tips_include_tip_hide(self->tips);
				break;
			}

			return false;
		}
	}

	GtkTextIter end = iter;
	switch( event->keyval ) {
	case '.':
		//show_hint(*page, it, end, 's');
		break;

	case ':':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)==':' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			//show_hint(*page, it, end, 's');
		}
		break;

	case '(':
		//show_hint(*page, it, end, 'f');
		break;

	case ')':
		tips_decl_tip_hide(self->tips);
		break;
	case '<':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)!='<' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			//show_hint(*page, it, end, 'f');
		}
		break;
	case '>':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)=='-' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			//show_hint(*page, it, end, 's');

		} else {
			tips_decl_tip_hide(self->tips);
		}
		break;
	default:
		if( (event->keyval <= 0x7f) && ::isalnum(event->keyval) || event->keyval=='_' ) {
			if( tips_list_is_visible(self->tips) ) {
				// locate_sub_hint(view);

			} else {
				// set_show_hint_timer(*page);
			}

		} else {
			tips_list_tip_hide(self->tips);
		}
		break;
	}

	return TRUE;
}

gboolean LJCS::on_button_release_event(GtkWidget* view, GdkEventButton* event, LJCS* self) {
    tips_hide_all(self->tips);

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
    tips_hide_all(self->tips);
	return FALSE;
}

gboolean LJCS::on_scroll_event(GtkWidget* view, GdkEventScroll* event, LJCS* self) {
    tips_hide_all(self->tips);
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

