// Controls.c
// 

#include "LanguageTips.h"

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

struct _ControlsPriv {
	// ui
	guint		merge_id;
	GtkAction*	show_function_action;
	GtkAction*	jump_to_define_action;

	// view signal handlers
	gulong		page_added_handler_id;
	gulong		page_removed_handler_id;

	// tips priv
	gint			tips_last_line;
	gint			tips_last_offset;

};

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

static void show_hint(LanguageTips* self, GtkTextView* view, GtkTextBuffer* buf, GtkTextIter* it, GtkTextIter* end, gchar tag);

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
		if( !paths )
			return;

		for( p=paths->path_list; !succeed && p; p=p->next ) {
			filepath = g_build_filename((gchar*)(p->data), filename, NULL);
			succeed = self->app->doc_open(filepath, -1, -1, FALSE);
			g_free(filepath);
		}
	}
}

static gboolean search_current(LanguageTips* self, GtkTextView* view, gpointer* out_spath, CppFile** out_file, gint* out_line) {
	GtkTextBuffer* buf;
	GString* url;
	GtkTextIter it;
	GtkTextIter end;
	gunichar ch;

	buf = gtk_text_view_get_buffer(view);
	url = self->app->doc_get_url(buf);
	if( !url )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));

	ch = gtk_text_iter_get_char(&it);
	if( !g_unichar_isalnum(ch) && ch!='_' ) {
		if( !gtk_text_iter_backward_char(&it) )
			return FALSE;
		ch = gtk_text_iter_get_char(&it);
		if( !g_unichar_isalnum(ch) && ch!='_' )
			return FALSE;
	}

	// find key end position
	while( gtk_text_iter_forward_char(&it) ) {
		ch = gtk_text_iter_get_char(&it);
		if( !g_unichar_isalnum(ch) && ch!='_' )
			break;
	}

	// find keys
	end = it;
	*out_line = gtk_text_iter_get_line(&it) + 1;

	*out_file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
	if( *out_file ) {
		*out_spath = find_spath_in_text_buffer(*out_file, &it, &end, FALSE);
		if( *out_spath )
			return TRUE;
		cpp_file_unref(*out_spath);
		*out_spath = 0;
	}

	return FALSE;
}

static void jump_to_current(LanguageTips* self, GtkTextView* view) {
	CppFile* file;
	gint line;
	gpointer spath;
	GSequence* seq;
	gboolean is_last;
	gint index;
	CppElem* elem;

	if( search_current(self, view, &spath, &file, &line) ) {
		seq = cpp_guide_search(self->cpp_guide, spath, 0, file, line, -1, -1);
		cpp_file_unref(file);

		if( !seq )
			return;

		is_last = self->jump_to_seq
			&& g_sequence_get_length(seq)==g_sequence_get_length(self->jump_to_seq)
			&& g_sequence_get(g_sequence_get_begin_iter(seq))==g_sequence_get(g_sequence_get_begin_iter(self->jump_to_seq));

		if( is_last ) {
			g_sequence_free(seq);
			seq = self->jump_to_seq;
			index = (self->jump_to_index + 1) % g_sequence_get_length(seq);
		} else {
			if( self->jump_to_seq )
				g_sequence_free(self->jump_to_seq);
			index = 0;
		}

		elem = (CppElem*)g_sequence_get( g_sequence_get_iter_at_pos(seq, index) );

		// skip current line elem
		if( elem->file==file && elem->sline==line )
			index = (index + 1) % g_sequence_get_length(seq);

		self->jump_to_seq = seq;
		self->jump_to_index = index;

		open_and_locate_elem(self, elem);
	}
}

static void show_current_in_preview(LanguageTips* self, GtkTextView* view) {
	CppFile* file;
	gint line;
	gpointer spath;

	if( search_current(self, view, &spath, &file, &line) ) {
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

gboolean on_show_hint_timeout(GtkTextView* view, LanguageTips* self) {
	GtkNotebook* doc_panel;
	gint page_num;
	GtkTextBuffer* buf;
	GtkTextIter iter;
	GtkTextIter end;

	g_assert( view );

	//if( show_hint_tag_!=tag )
	//	return false;

	doc_panel = puss_get_doc_panel(self->app);
	page_num = gtk_notebook_get_current_page(doc_panel);
	if( page_num < 0 || self->app->doc_get_view_from_page_num(page_num) != view )
		return FALSE;

	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	gtk_text_iter_backward_char(&iter);
	end = iter;
	show_hint(self, view, buf, &iter, &end, 's');

	return FALSE;
}

static void kill_show_hint_timer(LanguageTips* self) {
	//show_hint_timer_.disconnect();
}

static void set_show_hint_timer(LanguageTips* self, GtkTextView* view) {
	kill_show_hint_timer(self);

	//++show_hint_tag_;
	//show_hint_timer_ = Glib::signal_timeout().connect( sigc::bind(sigc::mem_fun(this, &LJCS::on_show_hint_timeout), &page, show_hint_tag_), 200 );

	on_show_hint_timeout(view, self);
}

static void locate_sub_hint(LanguageTips* self, GtkTextView* view) {
	GtkTextBuffer* buf;
	GtkTextIter iter;
	GtkTextIter end;
	gunichar ch;
	gchar* text;
	gint x;
	gint y;

	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	end = iter;

	while( gtk_text_iter_backward_char(&iter) ) {
		ch = gtk_text_iter_get_char(&iter);
		if( g_unichar_isalnum(ch) || ch=='_' )
			continue;

		gtk_text_iter_forward_char(&iter);
		break;
	}

	text = gtk_text_iter_get_text(&iter, &end);
	x = 0;
	y = 0;
	calc_tip_pos(view, &x, &y);
	if( !tips_locate_sub(self, x, y, text) )
		set_show_hint_timer(self, view);

	g_free(text);
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
	gint offset;
	gpointer spath;
	GSequence* seq;

	if( tag=='f' )
		tips_list_tip_hide(self);

	kill_show_hint_timer(self);

	url = self->app->doc_get_url(buf);
	if( !url )
		return;

	// find keys
	line = gtk_text_iter_get_line(it);
	offset = gtk_text_iter_get_line_offset(it);

	file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
	if( file ) {
		spath = find_spath_in_text_buffer(file, it, end, tag!='f');
		if( spath ) {
			flag = CPP_GUIDE_SEARCH_FLAG_USE_UNIQUE_ID;
			if( tag=='s' )
				flag |= CPP_GUIDE_SEARCH_FLAG_WITH_KEYWORDS;

			seq = cpp_guide_search( self->cpp_guide, spath, flag, file, line + 1, 100, 200);
			if( seq ) {
				gint x = 0;
				gint y = 0;
				calc_tip_pos(view, &x, &y);

				if( tag=='s' ) {
					tips_list_tip_show(self, x, y, seq);
					self->controls_priv->tips_last_line = line;
				} else {
					tips_decl_tip_show(self, x, y, seq);
					self->controls_priv->tips_last_offset = offset;
				}
			}

			cpp_spath_free(spath);
		}
		cpp_file_unref(file);
	}
}

static gboolean view_on_key_press(GtkTextView* view, GdkEventKey* event, LanguageTips* self) {
    if( event->state & GDK_MOD1_MASK ) {
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
			tips_select_prev(self);
			return TRUE;
		}

		if( tips_decl_is_visible(self) )
			tips_decl_tip_hide(self);
		break;

	case GDK_Down:
		if( tips_list_is_visible(self) || tips_include_is_visible(self) ) {
			tips_select_next(self);
			return TRUE;
		}

		if( tips_decl_is_visible(self) )
			tips_decl_tip_hide(self);
		break;

	case GDK_Escape:
		tips_hide_all(self);
		return TRUE;
		break;
	}

	return FALSE;
}

static gboolean view_on_key_release(GtkTextView* view, GdkEventKey* event, LanguageTips* self) {
    if( event->state & GDK_MOD1_MASK ) {
        tips_hide_all(self);
        return FALSE;
    }

    switch( event->keyval ) {
	case GDK_F12:
		jump_to_current(self, view);
		return FALSE;

	case GDK_Alt_L:
	case GDK_Alt_R:
		show_current_in_preview(self, view);
        return FALSE;
    }

	if( tips_is_visible(self) ) {
		switch( event->keyval ) {
		case GDK_Delete:
		case GDK_Left:
		case GDK_Right:
		case GDK_BackSpace:
		case GDK_Return:
		case GDK_KP_Enter:
			{
				GtkTextBuffer* buf;
				GtkTextIter iter;
				buf = gtk_text_view_get_buffer(view);
				gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

				if( gtk_text_iter_get_line(&iter)==self->controls_priv->tips_last_line ) {
					if( tips_list_is_visible(self) ) {
						locate_sub_hint(self, view);

					} else {
						set_show_hint_timer(self, view);
					}
				} else {
					tips_hide_all(self);
				}
			}
			break;
		}
	}

	return TRUE;
}

static void view_on_im_commit(GtkIMContext *cxt, gchar* str, LanguageTips* self) {
	gint page_num;
	GtkTextView* view;
	GtkTextBuffer* buf;
	GtkTextIter iter;
	GtkTextIter end;
	gunichar last_input;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	view = self->app->doc_get_view_from_page_num(page_num);
	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	gtk_text_iter_backward_char(&iter);
	last_input = gtk_text_iter_get_char(&iter);

	//g_print("cur : %d %s\n", gtk_text_iter_get_char(&iter), str);

	if( is_in_comment(&iter) )
		return;

	// include test
	//
	if( tips_include_is_visible(self) ) {
		if( show_include_files_tips(self, view, buf, &iter, last_input) )
			return;

	} else {
		switch( last_input ) {
		case '\"':
		case '<':
			if( show_include_files_tips(self, view, buf, &iter, last_input) )
				return;
			break;
		}
	}

	end = iter;
	switch( last_input ) {
	case '.':
		show_hint(self, view, buf, &iter, &end, 's');
		break;

	case ':':
		if( gtk_text_iter_backward_char(&iter) && gtk_text_iter_get_char(&iter)==':' ) {
			gtk_text_iter_forward_char(&iter);
			show_hint(self, view, buf, &iter, &end, 's');
		}
		break;

	case '(':
		show_hint(self, view, buf, &iter, &end, 'f');
		break;

	case ')':
		tips_decl_tip_hide(self);
		break;

	case '<':
		if( gtk_text_iter_backward_char(&iter) && gtk_text_iter_get_char(&iter)!='<' ) {
			gtk_text_iter_forward_char(&iter);
			show_hint(self, view, buf, &iter, &end, 'f');
		}
		break;

	case '>':
		if( gtk_text_iter_backward_char(&iter) && gtk_text_iter_get_char(&iter)=='-' ) {
			gtk_text_iter_forward_char(&iter);
			show_hint(self, view, buf, &iter, &end, 's');

		} else {
			tips_decl_tip_hide(self);
		}
		break;

	default:
		if( tips_decl_is_visible(self) ) {
			gboolean sign = FALSE;
			gint layer = 1;

			while( !sign && layer > 0 ) {
				gunichar ch = gtk_text_iter_get_char(&iter);
				switch(ch) {
				case '(':
					--layer;
					break;
				case ')':
					++layer;
					break;
				case ';':
				case '{':
				case '}':
					sign = TRUE;
					break;
				}

				if( !gtk_text_iter_backward_char(&iter) )
					sign = TRUE;
			}

			if( layer > 0 )
				sign = TRUE;

			if( !sign )
				if( self->controls_priv->tips_last_offset != (gtk_text_iter_get_line_offset(&iter)+1) )
					sign = TRUE;

			if( sign )
				tips_decl_tip_hide(self);

		} else {
			gboolean sign = FALSE;

			if( last_input=='_' || g_unichar_isalnum(last_input) )
				sign = TRUE;

			if( sign ) {
				if( tips_list_is_visible(self) ) {
					locate_sub_hint(self, view);

				} else {
					set_show_hint_timer(self, view);
				}

			} else {
				tips_list_tip_hide(self);
			}
		}
		break;
	}
}

static gboolean view_on_button_release(GtkTextView* view, GdkEventButton* event, LanguageTips* self) {
	tips_hide_all(self);

    // include test
	if( event->state & GDK_CONTROL_MASK ) {
		if( open_include_file_at_current(self, view) )
			return FALSE;
	}

	show_current_in_preview(self, view);

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

	g_signal_connect(view->im_context, "commit", G_CALLBACK(view_on_im_commit), self);

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

static const gchar* ui_info =
	"<ui>"
	"  <menubar name='main_menubar'>"
	"     <menu action='edit_menu'>"
	"      <placeholder name='edit_menu_plugins_place'>"
	"        <menuitem action='cpp_guide_show_function_action'/>"
	"        <menuitem action='cpp_guide_jump_to_define_action'/>"
	"      </placeholder>"
	"    </menu>"
	"  </menubar>"
	"</ui>"
	;

static void on_show_function_action(GtkAction* action, LanguageTips* self) {
	gboolean res = FALSE;
	gint page_num;
	GtkTextView* view;
	GtkWidget* actived;
	GtkTextBuffer* buf;
	GtkTextIter iter;
	GtkTextIter end;
	gint layer;
	gunichar ch;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	view = self->app->doc_get_view_from_page_num(page_num);
	actived = gtk_window_get_focus(puss_get_main_window(self->app));
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	layer = 1;
	while( layer > 0 && gtk_text_iter_backward_char(&iter) ) {
		ch = gtk_text_iter_get_char(&iter);
		switch(ch) {
		case '(':
			--layer;
			break;
		case ')':
			++layer;
			break;
		case ';':
		case '{':
		case '}':
			return;
		}
	}

	if( layer==0 ) {
		end = iter;
		show_hint(self, view, buf, &iter, &end, 'f');
	}
}

static void on_jump_to_define_action(GtkAction* action, LanguageTips* self) {
	gboolean res = FALSE;
	gint page_num;
	GtkTextView* view;
	GtkWidget* actived;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	view = self->app->doc_get_view_from_page_num(page_num);
	actived = gtk_window_get_focus(puss_get_main_window(self->app));
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	jump_to_current(self, view);
}

SIGNAL_CALLBACK gboolean tips_list_cb_query_tooltip( GtkTreeView* tree_view
	, gint x
	, gint y
	, gboolean keyboard_mode
	, GtkTooltip* tooltip
	, LanguageTips* self )
{
	CppElem* elem;
	GtkTreeModel* model;
	GtkTreePath* path;
	GtkTreeIter iter;
	if( gtk_tree_view_get_tooltip_context(tree_view, &x, &y, keyboard_mode, &model, &path, &iter) ) {
		elem = 0;
		gtk_tree_model_get(model, &iter, 2, &elem, -1);
		if( elem ) {
			gtk_tooltip_set_text(tooltip, elem->decl->buf);
			return TRUE;
		}
	}

	return FALSE;
}

void controls_init(LanguageTips* self) {
	ControlsPriv* priv;
	GtkNotebook* doc_panel;
	gint i;
	gint page_count;

	GtkActionGroup* main_action_group;
	GtkUIManager* ui_mgr;
	GError* err;

	priv = g_new0(ControlsPriv, 1);
	self->controls_priv = priv;

	priv->show_function_action = gtk_action_new("cpp_guide_show_function_action", _("function tip"), _("show function args tip."), GTK_STOCK_FIND);
	priv->jump_to_define_action = gtk_action_new("cpp_guide_jump_to_define_action", _("jump to define"), _("jump to define."), GTK_STOCK_JUMP_TO);
	g_signal_connect(priv->show_function_action, "activate", G_CALLBACK(on_show_function_action), self);
	g_signal_connect(priv->jump_to_define_action, "activate", G_CALLBACK(on_jump_to_define_action), self);

	main_action_group = GTK_ACTION_GROUP(gtk_builder_get_object(self->app->get_ui_builder(), "main_action_group"));
	gtk_action_group_add_action_with_accel(main_action_group, priv->show_function_action, "<Control><Shift>space");
	gtk_action_group_add_action_with_accel(main_action_group, priv->jump_to_define_action, "<Alt>g");

	ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(self->app->get_ui_builder(), "main_ui_manager"));

	err = 0;
	priv->merge_id = gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, &err);

	if( err ) {
		g_printerr("ERROR(language tips) : %s", err->message);
		g_error_free(err);
	}

	gtk_ui_manager_ensure_update(ui_mgr);

	doc_panel = puss_get_doc_panel(self->app);
	priv->page_added_handler_id = g_signal_connect(doc_panel, "page-added",   G_CALLBACK(on_doc_page_added), self);
	priv->page_removed_handler_id = g_signal_connect(doc_panel, "page-removed", G_CALLBACK(on_doc_page_removed), self);

	page_count = gtk_notebook_get_n_pages(doc_panel);
	for( i=0; i<page_count; ++i )
		signals_connect( self, self->app->doc_get_view_from_page_num(i) );

	self->update_timer = g_timeout_add(500, (GSourceFunc)on_update_timeout, self);
}

void controls_final(LanguageTips* self) {
	ControlsPriv* priv;
	GtkUIManager* ui_mgr;
	GtkActionGroup* group;

	gint i;
	gint page_count;
	GtkNotebook* doc_panel;

	priv = self->controls_priv;
	g_source_remove(self->update_timer);

	doc_panel = puss_get_doc_panel(self->app);
	g_signal_handler_disconnect(doc_panel, priv->page_added_handler_id);
	g_signal_handler_disconnect(doc_panel, priv->page_removed_handler_id);

	page_count = gtk_notebook_get_n_pages(doc_panel);
	for( i=0; i<page_count; ++i )
		signals_disconnect( self, self->app->doc_get_view_from_page_num(i) );

	group = GTK_ACTION_GROUP( gtk_builder_get_object(self->app->get_ui_builder(), "main_action_group") );
	ui_mgr = GTK_UI_MANAGER( gtk_builder_get_object(self->app->get_ui_builder(), "main_ui_manager") );

	gtk_ui_manager_remove_ui(ui_mgr, priv->merge_id);
	gtk_action_group_remove_action(group, priv->show_function_action);
	g_object_unref(priv->show_function_action);
}

