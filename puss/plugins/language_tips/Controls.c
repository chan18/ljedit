// Controls.c
// 

#include "LanguageTips.h"

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

static void calc_tip_pos(GtkTextView* view, gint* px, gint* py) {
	GtkTextIter iter;
	GdkRectangle rect;
	GtkTextBuffer* buf;
	GdkWindow* gdkwin;

	buf = gtk_text_view_get_buffer(view);
	gdkwin = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	gdk_window_get_origin(gdkwin, px, py);

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	gtk_text_view_get_iter_location(view, &iter, &rect);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT, rect.x, rect.y, &rect.x, &rect.y);

	*px += rect.x;
	*py += (rect.y + rect.height + 2);
}

static gchar text_buffer_iter_do_prev(GtkTextIter* pos) {
	return gtk_text_iter_backward_char(pos)
		? (gchar)gtk_text_iter_get_char(pos)
		: '\0';
}

static gchar text_buffer_iter_do_next(GtkTextIter* pos) {
	return gtk_text_iter_forward_char(pos)
		? (gchar)gtk_text_iter_get_char(pos)
		: '\0';
}

static gchar* find_spath_in_text_buffer( CppFile* file
	, GtkTextIter* it
    , GtkTextIter* end
	, gboolean find_startswith )
{
	return cpp_spath_find(find_startswith, text_buffer_iter_do_prev, text_buffer_iter_do_next, it, end);
}

static gboolean check_cpp_files(LanguageTips* self, const gchar* filename) {
	gboolean retval = FALSE;
	gchar* name = g_path_get_basename(filename);
	size_t len = strlen(name);
	gchar* ptr = name + len;
	gchar* p;
	for( ; ptr!=name && *ptr != '.'; --ptr );

	if( ptr!=name ) {
		// check ext filename is .c .h .cpp .hpp .cc .hh .inl ...
		++ptr;
		for(p=ptr; p!=(name+len); ++p)
			*p = g_ascii_tolower(*p);

		if( g_str_equal(ptr, "c")
			|| g_str_equal(ptr, "h")
			|| g_str_equal(ptr, "cc")
			|| g_str_equal(ptr, "hh")
			|| g_str_equal(ptr, "cpp")
			|| g_str_equal(ptr, "hpp")
			|| g_str_equal(ptr, "inl") )
		{
			retval = TRUE;
		}

	} else {
		// if no ext filename, check path in system header path
		//retval = in_include_path(filename);
	}
	g_free(name);

	return retval;
}

static gboolean is_in_comment(GtkTextIter* it) {
	GtkTextBuffer* buf = gtk_text_iter_get_buffer(it);
	GtkTextTagTable* tag_table = gtk_text_buffer_get_tag_table(buf);
	GtkTextTag* block_comment_tag = gtk_text_tag_table_lookup(tag_table, "Block Comment");
	GtkTextTag* line_comment_tag = gtk_text_tag_table_lookup(tag_table, "Line Comment");

	if( block_comment_tag && gtk_text_iter_has_tag(it, block_comment_tag) )
		return TRUE;

	if( line_comment_tag && gtk_text_iter_has_tag(it, line_comment_tag) )
		return TRUE;

    return FALSE;
}

static void open_include_file(LanguageTips* self, const gchar* filename, gboolean system_header, const gchar* owner_path) {
	gboolean succeed = FALSE;
	gchar* filepath;
	gchar* dirpath;

	if( owner_path && !system_header ) {
		dirpath = g_path_get_dirname(owner_path);
		filepath = g_build_filename(dirpath, filename, NULL);
		succeed = self->app->doc_open(filepath, -1, -1, FALSE);
		g_free(dirpath);
		g_free(filepath);
	}

	if( !succeed ) {
		GList* p;
		CppIncludePaths* paths;
		paths = cpp_guide_include_paths_ref(self->cpp_guide);
		for( p=paths->path_list; !succeed && p; p=p->next ) {
			filepath = g_build_filename((gchar*)(p->data), filename, NULL);
			succeed = self->app->doc_open(filepath, -1, -1, FALSE);
			g_free(filepath);
		}
	}
}

static void show_current_in_preview(LanguageTips* self, GtkTextView* view) {
	GtkTextBuffer* buf;
	GString* url;
	CppFile* file;
	GtkTextIter it;
	GtkTextIter end;
	gunichar ch;
	gint line;
	gpointer spath;

	buf = gtk_text_view_get_buffer(view);
	url = self->app->doc_get_url(buf);
	if( !url )
		return;

	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));

	ch = gtk_text_iter_get_char(&it);
	if( !g_unichar_isalnum(ch) && ch!='_' )
		return;

	// find key end position
	while( gtk_text_iter_forward_char(&it) ) {
		ch = gtk_text_iter_get_char(&it);
		if( !g_unichar_isalnum(ch) && ch!='_' )
			break;
	}

	// find keys
	end = it;
	line = gtk_text_iter_get_line(&it) + 1;

	file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
	if( file ) {
		spath = find_spath_in_text_buffer(file, &it, &end, FALSE);
		preview_set(self, spath, file, line);
		cpp_file_unref(file);
	}
}

static gboolean open_include_file_at_current(LanguageTips* self, GtkTextView* view) {
	GtkTextBuffer* buf;
	CppFile* file;
	GtkTextIter it;

	gchar* str;
	GMatchInfo* inc_info;
	gboolean is_include_line;
	gchar* inc_info_text;
	GtkTextIter ps;
	GtkTextIter pe;
	GMatchInfo* info = 0;

	gchar* sign;
	gchar* filename;
	GString* url;

    // include test
	buf = gtk_text_view_get_buffer(view);

	tips_hide_all(self);
	// tag test
	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
	if( is_in_comment(&it) )
		return FALSE;

	ps = it;
	pe = it;

	gtk_text_iter_set_line_offset(&ps, 0);
	if( !gtk_text_iter_ends_line(&pe) )
		gtk_text_iter_forward_to_line_end(&pe);

	str = gtk_text_iter_get_text(&ps, &pe);
	inc_info = 0;
	is_include_line = g_regex_match(self->re_include, str, (GRegexMatchFlags)0, &inc_info);

	if( is_include_line ) {
		inc_info_text = g_match_info_fetch(inc_info, 1);
		if( g_regex_match(self->re_include_info, inc_info_text, (GRegexMatchFlags)0, &info) ) {
			sign = g_match_info_fetch(info, 1);
			filename = g_match_info_fetch(info, 2);
			url = self->app->doc_get_url(buf);
			if( url ) {
				file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
				if( file ) {
					open_include_file(self, filename, *sign=='<', file->filename->buf);
				} else {
					GString* filepath = self->app->doc_get_url(buf);
					if( filepath )
						open_include_file(self, filename, *sign=='<', filepath->str);
				}
			}
			g_free(sign);
			g_free(filename);
		}
		g_free(inc_info_text);
		g_match_info_free(info);
	}
	g_match_info_free(inc_info);
	g_free(str);

	return is_include_line;
}

#ifdef G_OS_WIN32
	#define IS_DIR_SEPERATOR(ch) ((ch)=='\\' || (ch)=='/')
#else
	#define IS_DIR_SEPERATOR(ch) (ch)==G_DIR_SEPARATOR
#endif

static GList* search_include_files_in_dir(GList* files, gchar* inc_path, gchar* subpath, gchar* starts) {
	const gchar* fname;
	gchar* dirname = inc_path;
	GDir* dir;

	if( subpath )
		dirname = g_build_path(G_DIR_SEPARATOR_S, inc_path, subpath, 0);

	dir = g_dir_open(dirname, 0, 0);
	while(dir) {
		fname = g_dir_read_name(dir);
		if( !fname )
			break;
		if( *starts=='\0' || g_strrstr(fname, starts)==fname )
			files = g_list_append(files, g_strdup(fname));
	}
	g_dir_close(dir);

	if( subpath )
		g_free(dirname);

	return files;
}

static GList* search_include_files(CppGuide* guide, gchar* startswith, GString* current_url) {
	GList* files = 0;
	gint len;
	gchar* key;
	gchar* path;
	gchar* dirname;
	GList* p;

	g_assert( startswith );
	len = (gint)strlen(startswith);
	key = startswith;
	path = 0;

	if( len ) {
		for( key = startswith + len - 1; key>startswith; --key ) {
			if( IS_DIR_SEPERATOR(*key) ) {
				if( key!=startswith )
					path = startswith;

				*key = '\0';
				++key;
				break;
			}
		}
	}

	if( current_url ) {
		dirname = g_path_get_dirname(current_url->str);
		files = search_include_files_in_dir(files, dirname, path, key);
		g_free(dirname);

	} else {
		CppIncludePaths* paths = cpp_guide_include_paths_ref(guide);
		if( paths ) {
			for( p=paths->path_list; p; p=p->next )
				files = search_include_files_in_dir(files, (gchar*)(p->data), path, key);
			cpp_guide_include_paths_unref(paths);
		}
	}

	return files;
}

static void show_include_hint(LanguageTips* self, GtkTextView* view, gchar* startswith, GString* current_url) {
	gint x = 0;
	gint y = 0;
	GList* files = search_include_files(self->cpp_guide, startswith, current_url);
	if( files ) {
		calc_tip_pos(view, &x, &y);
		tips_include_tip_show(self, x, y, files);
	} else {
		tips_include_tip_hide(self);
	}
}

static gboolean show_include_files_tips(LanguageTips* self, GtkTextView* view, GtkTextBuffer* buf, GtkTextIter* iter, gint keyval) {
	gchar* line;
	gchar* inc_info_text;
	gchar* sign;
	gchar* filename;
	GString* url;
	GMatchInfo* inc_info;
	GMatchInfo* info;
	gboolean is_start;
	gboolean is_include_line;
	GtkTextIter ps = *iter;
	GtkTextIter pe = *iter;

	gtk_text_iter_set_line_offset(&ps, 0);
	if( !gtk_text_iter_ends_line(&pe) )
		gtk_text_iter_forward_to_line_end(&pe);

	line = gtk_text_iter_get_text(&ps, &pe);
	inc_info = 0;
	is_start = FALSE;
	is_include_line = g_regex_match(self->re_include, line, (GRegexMatchFlags)0, &inc_info);
	if( is_include_line ) {
		info = 0;
		inc_info_text = g_match_info_fetch(inc_info, 1);
		if( g_regex_match(self->re_include_tip, inc_info_text, (GRegexMatchFlags)0, &info) ) {
			sign = g_match_info_fetch(info, 1);
			filename = g_match_info_fetch(info, 2);
			if( filename[0]=='\0' )
				is_start = TRUE;

			url = (*sign=='<') ? 0 : self->app->doc_get_url(buf);
			show_include_hint(self, view, filename, url);

			g_free(sign);
			g_free(filename);
		}
		g_free(inc_info_text);
		g_match_info_free(info);
	}
	g_match_info_free(inc_info);
	g_free(line);

	if( is_include_line ) {
		switch( keyval ) {
		case '"':
			if( is_start )
				break;
		case '>':
			tips_include_tip_hide(self);
			break;
		}

		return TRUE;
	}

	return FALSE;
}

static void auto_complete(LanguageTips* self, GtkTextView* view) {
	CppElem* elem;
	const gchar* str;
	GtkTextBuffer* buf;
	GtkTextIter ps;
	GtkTextIter pe;
	gunichar ch;

	if( tips_list_is_visible(self) ) {
		elem = tips_list_get_selected(self);
		if( !elem )
			return;

		buf = gtk_text_view_get_buffer(view);
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
		pe = ps;

		ch = 0;

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
		gtk_text_buffer_insert_at_cursor(buf, elem->name->buf, tiny_str_len(elem->name));
		tips_list_tip_hide(self);

	} else if( tips_include_is_visible(self) ) {
		str = tips_include_get_selected(self);
		if( !str )
			return;

		buf = gtk_text_view_get_buffer(view);
		gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
		pe = ps;

		ch = 0;

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
		gtk_text_buffer_insert_at_cursor(buf, str, -1);
		tips_include_tip_hide(self);
	}
}

static void show_hint(LanguageTips* self, GtkTextView* view, GtkTextBuffer* buf, GtkTextIter* it, GtkTextIter* end, gchar tag) {
	GString* url;
	gint flag;
	CppFile* file;
	gint line;
	gpointer spath;
	GSequence* seq;

	if( tag=='f' )
		tips_list_tip_hide(self);

	//kill_show_hint_timer();

	url = self->app->doc_get_url(buf);
	if( !url )
		return;

	// find keys
	line = gtk_text_iter_get_line(it) + 1;

	file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
	if( file ) {
		spath = find_spath_in_text_buffer(file, it, end, TRUE);
		if( spath ) {
			flag = CPP_GUIDE_SEARCH_FLAG_USE_UNIQUE_ID;
			if( tag=='s' )
				flag |= CPP_GUIDE_SEARCH_FLAG_WITH_KEYWORDS;

			seq = cpp_guide_search( self->cpp_guide, spath, flag, file, line);
			if( seq ) {
				gint x = 0;
				gint y = 0;
				calc_tip_pos(view, &x, &y);
				tips_list_tip_show(self, x, y, seq);
			}

			cpp_spath_free(spath);
		}
		cpp_file_unref(file);
	}
}

static gboolean view_on_key_press(GtkTextView* view, GdkEventKey* event, LanguageTips* self) {
    if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) ) {
		tips_hide_all(self);
        return FALSE;
    }

	switch( event->keyval ) {
	case GDK_Tab:
		if( tips_list_is_visible(self) || tips_include_is_visible(self) ) {
			auto_complete(self, view);
			return TRUE;
		}
		break;

	case GDK_Return:
		if( tips_list_is_visible(self) || tips_include_is_visible(self) ) {
			tips_hide_all(self);
			auto_complete(self, view);
			return TRUE;
		}
		break;

	case GDK_Up:
		if( tips_list_is_visible(self) || tips_include_is_visible(self) ) {
			// tips_select_prev(self);
			return TRUE;
		}

		if( tips_decl_is_visible(self) )
			tips_decl_tip_hide(self);
		break;

	case GDK_Down:
		if( tips_list_is_visible(self) || tips_include_is_visible(self) ) {
			// tips_select_next(self);
			return TRUE;
		}

		if( tips_decl_is_visible(self) )
			tips_decl_tip_hide(self);
		break;

	case GDK_Escape:
		tips_hide_all(self);
		return TRUE;
	}

	return FALSE;
}

static gboolean view_on_key_release(GtkTextView* view, GdkEventKey* event, LanguageTips* self) {
	/*
	// test
	if( event->keyval==GDK_space ) {
		if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) )
			show_current_in_preview(self, view);
		return TRUE;
	}
	*/

	GtkTextIter iter;
	GtkTextIter end;
	GtkTextBuffer* buf;

    if( event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK) ) {
        //tips_hide_all(self);
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

	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	if( is_in_comment(&iter) )
		return FALSE;

	// include test
	//
	//if( include_tips.visible )
	if( FALSE ) {
		if( show_include_files_tips(self, view, buf, &iter, event->keyval) )
			return FALSE;

	} else {
		switch( event->keyval ) {
		case '\"':
		case '<':
			if( show_include_files_tips(self, view, buf, &iter, event->keyval) )
				return FALSE;
			break;
		}
	}
	end = iter;
	switch( event->keyval ) {
	case '.':
		show_hint(self, view, buf, &iter, &end, 's');
		break;

	case ':':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)==':' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			show_hint(self, view, buf, &iter, &end, 's');
		}
		break;

	case '(':
		show_hint(self, view, buf, &iter, &end, 'f');
		break;

	case ')':
		//tips_decl_tip_hide(self);
		break;

	case '<':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)!='<' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			show_hint(self, view, buf, &iter, &end, 'f');
		}
		break;

	case '>':
		if( gtk_text_iter_backward_chars(&iter, 2) && gtk_text_iter_get_char(&iter)=='-' ) {
			gtk_text_iter_forward_chars(&iter, 2);
			show_hint(self, view, buf, &iter, &end, 's');

		} else {
			tips_decl_tip_hide(self);
		}
		break;

	default:
		if( (event->keyval <= 0x7f) && g_ascii_isalnum(event->keyval) || event->keyval=='_' ) {
			/*
			if( tips_list_is_visible(self) ) {
				self->locate_sub_hint(view);

			} else {
				self->set_show_hint_timer(view);
			}
			*/
			show_hint(self, view, buf, &iter, &end, 's');

		} else {
			tips_list_tip_hide(self);
		}
		break;
	}

	return TRUE;
}

static gboolean view_on_button_release(GtkTextView* view, GdkEventButton* event, LanguageTips* self) {
    // include test
	if( event->state & GDK_CONTROL_MASK ) {
		if( open_include_file_at_current(self, view) )
			return FALSE;
	}

	// test CTRL state
	if( event->state & GDK_CONTROL_MASK ) {
		// jump to
		/*
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
		*/

	} else {
		// preview
		show_current_in_preview(self, view);
	}


    return FALSE;
}

static gboolean view_on_focus_out(GtkTextView* view, GdkEventFocus* event, LanguageTips* self) {
	tips_hide_all(self);
	return FALSE;
}

static gboolean view_on_scroll(GtkTextView* view, GdkEventScroll* event, LanguageTips* self) {
	tips_hide_all(self);
	return FALSE;
}

static void buf_on_modified_changed(GtkTextBuffer* buf, LanguageTips* self) {
	GString* url = self->app->doc_get_url(buf);
	parse_thread_push(self, url ? url->str : 0, FALSE);
}

static void signals_connect(LanguageTips* self, GtkTextView* view) {
	GString* url;
	GtkTextBuffer* buf;
	GtkSourceLanguage* lang;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	url = self->app->doc_get_url(buf);
	if( !url )
		return;

	if( !check_cpp_files(self, url->str) )
		return;

	g_signal_connect(view, "key-press-event", G_CALLBACK(view_on_key_press), self);
	g_signal_connect(view, "key-release-event", G_CALLBACK(view_on_key_release), self);

	g_signal_connect(view, "button-release-event", G_CALLBACK(view_on_button_release), self);
	g_signal_connect(view, "focus-out-event", G_CALLBACK(view_on_focus_out), self);
	g_signal_connect(view, "scroll-event", G_CALLBACK(view_on_scroll), self);

	g_signal_connect(buf, "modified-changed", G_CALLBACK(&buf_on_modified_changed), self);

	lang = gtk_source_buffer_get_language(GTK_SOURCE_BUFFER(buf));
	if( !lang ) {
		GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
		GtkSourceLanguage* cpp_lang = gtk_source_language_manager_get_language(lm, "cpp");
		gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buf), cpp_lang);
	}

	parse_thread_push(self, url->str, FALSE);
}

static void signals_disconnect(LanguageTips* self, GtkTextView* view) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

    g_signal_handlers_disconnect_matched( view, (GSignalMatchType)(G_SIGNAL_MATCH_DATA), 0, 0, NULL, NULL, self );
    g_signal_handlers_disconnect_matched( buf, (GSignalMatchType)(G_SIGNAL_MATCH_DATA), 0, 0, NULL, NULL, self );
}

static void on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LanguageTips* self) {
	GtkTextView* view = self->app->doc_get_view_from_page(page);
	if( view )
		signals_connect(self, view);
}

static void on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LanguageTips* self) {
}

static gboolean on_update_timeout(LanguageTips* self) {
	outline_update(self);

	return TRUE;
}

void controls_init(LanguageTips* self) {
	GtkNotebook* doc_panel;
	gint i;
	gint page_count;

	doc_panel = puss_get_doc_panel(self->app);
	self->page_added_handler_id = g_signal_connect(doc_panel, "page-added",   G_CALLBACK(on_doc_page_added), self);
	self->page_removed_handler_id = g_signal_connect(doc_panel, "page-removed", G_CALLBACK(on_doc_page_removed), self);

	page_count = gtk_notebook_get_n_pages(doc_panel);
	for( i=0; i<page_count; ++i )
		signals_connect( self, self->app->doc_get_view_from_page_num(i) );

	self->update_timer = g_timeout_add(500, (GSourceFunc)on_update_timeout, self);
}

void controls_final(LanguageTips* self) {
	gint i;
	gint page_count;
	GtkNotebook* doc_panel;

	g_source_remove(self->update_timer);

	doc_panel = puss_get_doc_panel(self->app);
	g_signal_handler_disconnect(doc_panel, self->page_added_handler_id);
	g_signal_handler_disconnect(doc_panel, self->page_removed_handler_id);

	page_count = gtk_notebook_get_n_pages(doc_panel);
	for( i=0; i<page_count; ++i )
		signals_disconnect( self, self->app->doc_get_view_from_page_num(i) );
}

