// LJCS.cpp
// 

#include "LJCS.h"

#include "PreviewPage.h"
#include "OutlinePage.h"
#include "Tips.h"

#include <cstring>
#include <gdk/gdkkeysyms.h>

#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

GRegex* re_include = g_regex_new("^[ \t]*#[ \t]*include[ \t]*(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
GRegex* re_include_tip = g_regex_new("([\"<])(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
GRegex* re_include_info = g_regex_new("([\"<])([^\">]*)[\">].*", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

void calc_tip_pos(GtkTextView* view, gint& x, gint& y) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GdkWindow* gdkwin = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	gdk_window_get_origin(gdkwin, &x, &y);

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	GdkRectangle rect;
	gtk_text_view_get_iter_location(view, &iter, &rect);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT, rect.x, rect.y, &rect.x, &rect.y);

	x += rect.x;
	y += (rect.y + rect.height + 2);
}

LJCS::LJCS() : app(0) {}

LJCS::~LJCS() { destroy(); }

gboolean ljcs_update_timeout(LJCS* self) {
	preview_page_update(self->preview_page);
	outline_page_update(self->outline_page);

	return TRUE;
}

void parse_include_path_option(const Option* option, Environ* env) {
	StrVector paths;

	gchar** items = g_strsplit_set(option->value, ",; \t\r\n", 0);
	for( gchar** p=items; *p; ++p ) {
		if( *p[0]=='\0' )
			continue;

		if( g_str_has_suffix(*p, ".c")
			|| g_str_has_suffix(*p, ".cpp")
			|| g_str_has_suffix(*p, ".cc") )
		{
			continue;
		}

		paths.push_back(*p);

	}
	g_strfreev(items);
	env->set_include_paths(paths);
}

bool LJCS::create(Puss* _app) {
	app = _app;
	env.set_app(app);

	const Option* option = app->option_manager_option_reg("cpp_helper", "include_path", "/usr/include\n/usr/include/c++/4.2\n", 0, (gpointer)"text", 0);
	app->option_manager_monitor_reg(option, (OptionChanged)&parse_include_path_option, &env, 0);
	parse_include_path_option(option, &env);

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
		succeed = app->doc_open(filepath, -1, -1, FALSE);
		g_free(dirpath);
		g_free(filepath);
	}

	StrVector paths = env.get_include_paths();
	StrVector::iterator it = paths.begin();
	StrVector::iterator end = paths.end();
	for( ; !succeed && it!=end; ++it ) {
		gchar* filepath = g_build_filename(it->c_str(), filename, NULL);
		succeed = app->doc_open(filepath, -1, -1, FALSE);
		g_free(filepath);
	}
}

void LJCS::do_button_release_event(GtkTextView* view, GtkTextBuffer* buf, GdkEventButton* event, cpp::File* file) {
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
		if( !gtk_text_iter_ends_line(&pe) )
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

	gunichar ch = gtk_text_iter_get_char(&it);
    if( !g_unichar_isalnum(ch) && ch!='_' )
		return;

	// find key end position
	while( gtk_text_iter_forward_char(&it) ) {
        ch = gtk_text_iter_get_char(&it);
        if( !g_unichar_isalnum(ch) && ch!='_' )
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
				app->doc_open(elem->file.filename.c_str(), int(elem->sline - 1), 0, FALSE);
		}


	} else {
		// preview
		DocIter_Gtk ps(&it);
		DocIter_Gtk pe(&end);
		std::string key;
		if( !find_key(key, ps, pe, false) )
			return;

		gchar* key_text = gtk_text_iter_get_text(&it, &end);
		preview_page_preview(preview_page, key.c_str(), key_text, *file, line);
		g_free(key_text);
	}
}

void LJCS::show_include_hint(gchar* filename, gboolean system_header, GtkTextView* view) {
	g_assert( filename );
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	size_t len = strlen(filename);
	gchar* key = filename;
	gchar* path =0;

	if( len ) {
		for( key = filename + len - 1; key>filename; --key ) {
			if( *key==G_DIR_SEPARATOR ) {
				if( key!=filename )
					path = filename;

				*key = '\0';
				++key;
				break;
			}
		}
	}

	StringSet files;

	if( system_header ) {
		StrVector paths = env.get_include_paths();
		StrVector::iterator it = paths.begin();
		StrVector::iterator end = paths.end();
		for( ; it!=end; ++it ) {
			GDir* dir = 0;
			if( path ) {
				gchar* str = g_build_filename(it->c_str(), path, NULL);
				dir = g_dir_open(str, 0, 0);
				g_free(str);

			} else {
				dir = g_dir_open(it->c_str(), 0, 0);
			}

			while(dir) {
				const gchar* fn = g_dir_read_name(dir);
				if( !fn )
					break;

				if( len==0 || g_strrstr(fn, key)==fn )
					files.insert(fn);
			} 

			g_dir_close(dir);
		}

	} else {
		GString* url = app->doc_get_url(buf);
		GDir* dir = 0;
		if( !url )
			return;
		gchar* str = g_path_get_dirname(url->str);
		dir = g_dir_open(str, 0, 0);
		g_free(str);

		while(dir) {
			const gchar* fn = g_dir_read_name(dir);
			if( !fn )
				break;

			if( len==0 || g_strrstr(fn, key)==fn )
				files.insert(fn);
		} 

		g_dir_close(dir);
	}

	if( !files.empty() ) {
		gint x = 0;
		gint y = 0;
		calc_tip_pos(view, x, y);
		tips_include_tip_show(tips, x, y, files);

	} else {
		tips_include_tip_hide(tips);
	}
}

void LJCS::show_hint(GtkTextIter* it, GtkTextIter* end, char tag, GtkTextView* view) {
	if( tag=='f' )
		tips_list_tip_hide(tips);

	kill_show_hint_timer();

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	GString* url = app->doc_get_url(buf);
	if( !url )
		return;

	cpp::File* file = env.find_parsed(url->str);
	if( !file )
		return;

    size_t line = (size_t)gtk_text_iter_get_line(it) + 1;
    StrVector keys;
    DocIter_Gtk ps(it);
    DocIter_Gtk pe(end);
	if( find_keys(keys, it, end, file, true) ) {
		MatchedSet mset(env);
		if( env.stree().reader_try_lock() ) {
			search_keys(keys, mset, env.stree().ref(), file, line);
			env.stree().reader_unlock();

			gint x = 0;
			gint y = 0;
			calc_tip_pos(view, x, y);

			if( tag=='s' ) {
				gchar* str = gtk_text_iter_get_text(it, end);
				std::string key = str;
				g_free(str);

				env.find_keyword(key, mset);
				if( mset.size()==1 && key==(*mset.begin())->name )
					mset.clear();

				tips_list_tip_show(tips, x, y, mset.elems());

			} else {
				tips_decl_tip_show(tips, x, y, mset.elems());
			}
		}

	}

	env.file_decref(file);
}

void LJCS::locate_sub_hint(GtkTextView* view) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	GtkTextIter end = iter;

	while( gtk_text_iter_backward_char(&iter) ) {
		gunichar ch = gtk_text_iter_get_char(&iter);
		if( g_unichar_isalnum(ch) || ch=='_' )
			continue;

		gtk_text_iter_forward_char(&iter);
		break;
	}

	gchar* text = gtk_text_iter_get_text(&iter, &end);
	gint x = 0;
	gint y = 0;
	calc_tip_pos(view, x, y);
	if( !tips_locate_sub(tips, x, y, text) )
		set_show_hint_timer(view);
	g_free(text);
}

void LJCS::auto_complete(GtkTextView* view) {
	if( tips_list_is_visible(tips) ) {
		cpp::Element* selected = tips_list_get_selected(tips);
		if( !selected )
			return;

		GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
		GtkTextIter ps;
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
		GtkTextIter pe = ps;

		gunichar ch = 0;

		// range start
		while( gtk_text_iter_backward_char(&ps) ) {
			ch = gtk_text_iter_get_char(&ps);
			if( g_unichar_isalnum(ch) || ch=='_' )
				continue;

			gtk_text_iter_forward_char(&ps);
			break;
		}

		// range end
		do {
			ch = gtk_text_iter_get_char(&pe);
			if( g_unichar_isalnum(ch) || ch=='_' )
				continue;
			break;
		} while( gtk_text_iter_forward_char(&pe) );

		gtk_text_buffer_delete(buf, &ps, &pe);
		gtk_text_buffer_insert_at_cursor(buf, selected->name.c_str(), (gint)selected->name.size());
		tips_list_tip_hide(tips);

	} else if( tips_include_is_visible(tips) ) {
		const gchar* selected = tips_include_get_selected(tips);
		if( !selected )
			return;

		GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
		GtkTextIter ps;
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
		GtkTextIter pe = ps;

		gunichar ch = 0;

		// range start
		while( gtk_text_iter_backward_char(&ps) ) {
			ch = gtk_text_iter_get_char(&ps);
			if( ch!='/' && ch!='\\' && ch!='<' && ch!='"' )
				continue;

			gtk_text_iter_forward_char(&ps);
			break;
		}

		// range end
		do {
			ch = gtk_text_iter_get_char(&pe);
			if( g_unichar_isalnum(ch) || ch=='_' )
				continue;
			break;
		} while( gtk_text_iter_forward_char(&pe) );

		gtk_text_buffer_delete(buf, &ps, &pe);
		gtk_text_buffer_insert_at_cursor(buf, selected, -1);
		tips_include_tip_hide(tips);
	}
}

void LJCS::set_show_hint_timer(GtkTextView* view) {
	kill_show_hint_timer();

	//++show_hint_tag_;
	//show_hint_timer_ = Glib::signal_timeout().connect( sigc::bind(sigc::mem_fun(this, &LJCS::on_show_hint_timeout), &page, show_hint_tag_), 200 );

	on_show_hint_timeout(view, 0);
}

void LJCS::kill_show_hint_timer() {
	//show_hint_timer_.disconnect();
}

gboolean LJCS::on_key_press_event(GtkTextView* view, GdkEventKey* event, LJCS* self) {
    if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) ) {
		tips_hide_all(self->tips);
        return FALSE;
    }

	switch( event->keyval ) {
	case GDK_Tab:
		if( tips_list_is_visible(self->tips) || tips_include_is_visible(self->tips) ) {
			self->auto_complete(view);
			return TRUE;
		}
		break;

	case GDK_Return:
		if( tips_list_is_visible(self->tips) || tips_include_is_visible(self->tips) ) {
			tips_hide_all(self->tips);
			self->auto_complete(view);
			return TRUE;
		}
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

gboolean LJCS::on_show_hint_timeout(GtkTextView* view, gint tag) {
	g_assert( view );

	//if( show_hint_tag_!=tag )
	//	return false;

	GtkNotebook* doc_panel = puss_get_doc_panel(app);
	gint page_num = gtk_notebook_get_current_page(doc_panel);
	if( page_num < 0 || app->doc_get_view_from_page_num(page_num) != view )
		return FALSE;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextIter iter, end;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	end = iter;
	show_hint(&iter, &end, 's', view);

	return FALSE;
}

gboolean LJCS::on_key_release_event(GtkTextView* view, GdkEventKey* event, LJCS* self) {
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

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	if( is_in_comment(&iter) )
		return FALSE;

	// include tip
	{
		GtkTextIter ps = iter;
		GtkTextIter pe = iter;
		gtk_text_iter_set_line_offset(&ps, 0);
		if( !gtk_text_iter_ends_line(&pe) )
			gtk_text_iter_forward_to_line_end(&pe);

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

				self->show_include_hint(filename, *sign=='<', view);

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
		self->show_hint(&iter, &end, 's', view);
		break;

	case ':':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)==':' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			self->show_hint(&iter, &end, 's', view);
		}
		break;

	case '(':
		self->show_hint(&iter, &end, 'f', view);
		break;

	case ')':
		tips_decl_tip_hide(self->tips);
		break;

	case '<':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)!='<' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			self->show_hint(&iter, &end, 'f', view);
		}
		break;

	case '>':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)=='-' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			self->show_hint(&iter, &end, 's', view);

		} else {
			tips_decl_tip_hide(self->tips);
		}
		break;

	default:
		if( (event->keyval <= 0x7f) && ::isalnum(event->keyval) || event->keyval=='_' ) {
			if( tips_list_is_visible(self->tips) ) {
				self->locate_sub_hint(view);

			} else {
				self->set_show_hint_timer(view);
			}

		} else {
			tips_list_tip_hide(self->tips);
		}
		break;
	}

	return TRUE;
}

gboolean LJCS::on_button_release_event(GtkTextView* view, GdkEventButton* event, LJCS* self) {
    tips_hide_all(self->tips);

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GString* url = self->app->doc_get_url(buf);
	if( url ) {
		cpp::File* file = self->env.find_parsed(std::string(url->str, url->len));
		self->do_button_release_event(view, buf, event, file);
		if( file )
			self->env.file_decref(file);
	}

    return FALSE;
}

gboolean LJCS::on_focus_out_event(GtkTextView* view, GdkEventFocus* event, LJCS* self) {
    tips_hide_all(self->tips);
	return FALSE;
}

gboolean LJCS::on_scroll_event(GtkTextView* view, GdkEventScroll* event, LJCS* self) {
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

	GString* url = self->app->doc_get_url(buf);
	if( !url )
		return;

	std::string filepath(url->str, url->len);
	if( !self->env.check_cpp_files(filepath) )
		return;

	g_signal_connect(view, "key-press-event", G_CALLBACK(&LJCS::on_key_press_event), self);
	g_signal_connect(view, "key-release-event", G_CALLBACK(&LJCS::on_key_release_event), self);

	g_signal_connect(view, "button-release-event", G_CALLBACK(&LJCS::on_button_release_event), self);
	g_signal_connect(view, "focus-out-event", G_CALLBACK(&LJCS::on_focus_out_event), self);
	g_signal_connect(view, "scroll-event", G_CALLBACK(&LJCS::on_scroll_event), self);

	g_signal_connect(buf, "modified-changed", G_CALLBACK(&LJCS::on_modified_changed), self);

	GtkSourceLanguage* lang = gtk_source_buffer_get_language(GTK_SOURCE_BUFFER(buf));
	if( !lang ) {
		GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
		GtkSourceLanguage* cpp_lang = gtk_source_language_manager_get_language(lm, "cpp");
		gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buf), cpp_lang);
	}

	self->parse_thread.add(filepath);
}

void LJCS::on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self) {
}
