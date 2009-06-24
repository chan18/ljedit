// Controls.c
// 

#include "LanguageTips.h"

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

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
		if( spath ) {
			preview_set(self, spath, file, line);
			cpp_spath_free(spath);
		}
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

	// tips_hide_all(self->tips);
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

static gboolean show_include_files_tips(LanguageTips* self, GtkTextView* view, GtkTextBuffer* buf, GtkTextIter* iter, gint keyval) {
	gchar* line;
	gchar* inc_info_text;
	gchar* sign;
	gchar* filename;
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

			//self->show_include_hint(filename, *sign=='<', view);

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
			//tips_include_tip_hide(self->tips);
			break;
		}

		return TRUE;
	}

	return FALSE;
}

static void print_matched(CppElem* elem, LanguageTips* self) {
	g_print("matched : %s\n", elem->name->buf);
}

static void show_hint(LanguageTips* self, GtkTextView* view, GtkTextBuffer* buf, GtkTextIter* it, GtkTextIter* end, gchar tag) {
	GString* url;
	gint flag;
	CppFile* file;
	gint line;
	gpointer spath;
	GSequence* seq;

	//if( tag=='f' )
	//	tips_list_tip_hide(tips);

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
				//gint x = 0;
				//gint y = 0;
				//calc_tip_pos(view, x, y);
				// tips_list_tip_show(tips, x, y, mset.elems());

				g_sequence_foreach(seq, print_matched, self);
				g_sequence_free(seq);
			}

			cpp_spath_free(spath);
		}
		cpp_file_unref(file);
	}
}

static gboolean view_on_key_press(GtkTextView* view, GdkEventKey* event, LanguageTips* self) {
	/*
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
	*/

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
        //tips_hide_all(self->tips);
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
		//tips_decl_tip_hide(self->tips);
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
			//tips_decl_tip_hide(self->tips);
		}
		break;

	default:
		if( (event->keyval <= 0x7f) && g_ascii_isalnum(event->keyval) || event->keyval=='_' ) {
			/*
			if( tips_list_is_visible(self->tips) ) {
				self->locate_sub_hint(view);

			} else {
				self->set_show_hint_timer(view);
			}
			*/
			show_hint(self, view, buf, &iter, &end, 's');

		} else {
			//tips_list_tip_hide(self->tips);
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
    //tips_hide_all(self->tips);
	return FALSE;
}

static gboolean view_on_scroll(GtkTextView* view, GdkEventScroll* event, LanguageTips* self) {
    //tips_hide_all(self->tips);
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

