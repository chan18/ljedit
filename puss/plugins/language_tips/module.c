// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

#include <libintl.h>

#include "IPuss.h"

#include "cpp/guide.h"

#define TEXT_DOMAIN "language_tips"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct {
	Puss* app;

	CppGuide*		cpp_guide;

	GAsyncQueue*	parse_queue;
	GThread*		parse_thread;

	GtkBuilder*		builder;

	// outline window
	GtkWidget*		outline_panel;
	GtkTreeView*	outline_view;
	GtkTreeStore*	outline_store;
	CppFile*		outline_file;
	gint			outline_pos;

	// preview window
	GtkWidget*		preview_panel;
	GtkLabel*		preview_filename_label;
	GtkButton*		preview_number_button;
	GtkTextView*	preview_view;

	// tips window
	GtkWidget*		tips_include_window;
	GtkTreeView*	tips_include_view;
	GtkTreeModel*	tips_include_model;
	//StringSet*		tips_include_files;

	GtkWidget*		tips_list_window;
	GtkTreeView*	tips_list_view;
	GtkTreeModel*	tips_list_model;

	GtkWidget*		tips_decl_window;
	GtkTextView*	tips_decl_view;
	GtkTextBuffer*	tips_decl_buffer;

	// update timer
	guint			update_timer;

	gulong			page_added_handler_id;
	gulong			page_removed_handler_id;

	// regex
	GRegex*			re_include;
	GRegex*			re_include_tip;
	GRegex*			re_include_info;

} LanguageTips;

static LanguageTips* g_self = 0;

static gchar* PARSE_THREAD_EXIT_SIGN = "";

static gpointer tips_parse_thread(gpointer args) {
	CppFile* file;
	gchar* filename = 0;
	GAsyncQueue* queue = (GAsyncQueue*)args;
	if( !queue )
		return 0;

	while( (filename = (gchar*)g_async_queue_pop(queue)) != PARSE_THREAD_EXIT_SIGN ) {
		// parse file
		file = cpp_guide_parse(g_self->cpp_guide, filename, -1);
		if( file )
			cpp_file_unref(file);

		g_free(filename);
	}

	g_async_queue_unref(queue);

	return 0;
}

typedef struct {
	GtkTreeStore*	store;
	GtkTreeIter*	iter;
} AddItemTag;

static void outline_add_elem(CppElem* elem, AddItemTag* parent) {
	GtkTreeIter iter;

	if( elem->type==CPP_ET_INCLUDE || elem->type==CPP_ET_UNDEF )
		return;

	gtk_tree_store_append(parent->store, &iter, parent->iter);
	gtk_tree_store_set( parent->store, &iter
		//, 0, icons_->get_icon_from_elem(*elem)
		, 1, elem->name->buf
		, 2, elem
		, -1 );

	if( cpp_elem_has_subscope(elem) ) {
		AddItemTag tag = {parent->store, &iter};
		g_list_foreach( cpp_elem_get_subscope(elem), (GFunc)outline_add_elem, &tag );
	}
}

static void outline_set_file(LanguageTips* self, CppFile* file, gint line) {
	if( file != self->outline_file ) {
		gtk_tree_view_set_model(self->outline_view, 0);
		gtk_tree_store_clear(self->outline_store);

		if( self->outline_file ) {
			cpp_file_unref(self->outline_file);
			self->outline_file = 0;
		}

		if( file ) {
			AddItemTag tag = {self->outline_store, 0};

			self->outline_file = cpp_file_ref(file);
			g_list_foreach( file->root_scope.v_ncscope.scope, (GFunc)outline_add_elem, &tag );
		}

		gtk_tree_view_set_model(self->outline_view, GTK_TREE_MODEL(self->outline_store));
	}

	if( file && self->outline_pos != line ) {
		self->outline_pos = line;
		gtk_tree_selection_unselect_all( gtk_tree_view_get_selection(self->outline_view) );

		// locate_line( (size_t)line + 1, 0 );
	}
}

static void create_ui(LanguageTips* self) {
	gchar* filepath;
	GtkBuilder* builder;
	GError* err = 0;

	builder = gtk_builder_new();
	if( !builder )
		return;

	self->builder = builder;

	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	filepath = g_build_filename(self->app->get_plugins_path(), "language_tips.ui", NULL);
	if( !filepath ) {
		g_printerr("ERROR(search_tools) : build ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(search_tools): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	self->outline_panel = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
	self->outline_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "outline_treeview"));
	self->outline_store = GTK_TREE_STORE(g_object_ref(gtk_builder_get_object(builder, "outline_store")));
	g_assert( self->outline_panel && self->outline_view && self->outline_store );

	gtk_widget_show_all(self->outline_panel);
	self->app->panel_append(self->outline_panel, gtk_label_new(_("Outline")), "dev_outline", PUSS_PANEL_POS_RIGHT);

	self->preview_panel = GTK_WIDGET(gtk_builder_get_object(builder, "preview_panel"));
	self->preview_filename_label = GTK_LABEL(gtk_builder_get_object(builder, "filename_label"));
	self->preview_number_button = GTK_BUTTON(gtk_builder_get_object(builder, "number_button"));
	self->preview_view           = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "preview_view"));
	g_assert( self->preview_panel && self->preview_filename_label && self->preview_number_button && self->preview_view );

	gtk_widget_show_all(self->preview_panel);
	self->app->panel_append(self->preview_panel, gtk_label_new(_("Preview")), "dev_preview", PUSS_PANEL_POS_BOTTOM);

	self->tips_include_window = GTK_WIDGET(gtk_builder_get_object(builder, "include_window"));
	self->tips_include_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "include_view"));
	self->tips_include_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "include_store"));
	g_assert( self->tips_include_window && self->tips_include_view && self->tips_include_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "include_panel")));

	self->tips_list_window = GTK_WIDGET(gtk_builder_get_object(builder, "list_window"));
	self->tips_list_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "list_view"));
	self->tips_list_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "list_store"));
	g_assert( self->tips_list_window && self->tips_list_view && self->tips_list_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "list_panel")));

	self->tips_decl_window = GTK_WIDGET(gtk_builder_get_object(builder, "decl_window"));
	self->tips_decl_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "decl_view"));
	self->tips_decl_buffer = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(self->tips_decl_view));
	g_assert( self->tips_decl_window && self->tips_decl_view && self->tips_decl_buffer );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "decl_panel")));

	gtk_builder_connect_signals(builder, self);
}

static void destroy_ui(LanguageTips* self) {
	if( !self->builder )
		return;

	self->app->panel_remove(self->outline_panel);
	self->app->panel_remove(self->preview_panel);

	g_object_unref(G_OBJECT(self->builder));
}

static void outline_update(LanguageTips* self) {
	GtkNotebook* doc_panel;
	gint num;
	GtkTextBuffer* buf;
	GString* url;
	CppFile* file;
	GtkTextIter iter;

	doc_panel = puss_get_doc_panel(self->app);
	num = gtk_notebook_get_current_page(doc_panel);
	if( num < 0 )
		return;

	buf = self->app->doc_get_buffer_from_page_num(num);
	if( !buf )
		return;

	url = self->app->doc_get_url(buf);
	if( !url )
		return;

	file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
	if( !file )
		return;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	num = gtk_text_iter_get_line(&iter);
	outline_set_file(self, file, num);

	cpp_file_unref(file);
}

static gboolean on_update_timeout(LanguageTips* self) {
	outline_update(self);

	return TRUE;
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
		IncludePaths* paths;
		paths = cpp_guide_include_paths_ref(self->cpp_guide);
		for( p=paths->path_list; !succeed && p; p=p->next ) {
			filepath = g_build_filename((gchar*)(p->data), filename, NULL);
			succeed = self->app->doc_open(filepath, -1, -1, FALSE);
			g_free(filepath);
		}
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

	// include tip
	{
		gchar* line;
		gchar* inc_info_text;
		GMatchInfo* inc_info;
		gboolean is_start;
		gboolean is_include_line;
		GtkTextIter ps = iter;
		GtkTextIter pe = iter;

		gtk_text_iter_set_line_offset(&ps, 0);
		if( !gtk_text_iter_ends_line(&pe) )
			gtk_text_iter_forward_to_line_end(&pe);

		line = gtk_text_iter_get_text(&ps, &pe);
		inc_info = 0;
		is_start = FALSE;
		is_include_line = g_regex_match(self->re_include, line, (GRegexMatchFlags)0, &inc_info);
		if( is_include_line ) {
			GMatchInfo* info = 0;
			inc_info_text = g_match_info_fetch(inc_info, 1);
			if( g_regex_match(self->re_include_tip, inc_info_text, (GRegexMatchFlags)0, &info) ) {
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

			return FALSE;
		}
	}

	end = iter;
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
		if( (event->keyval <= 0x7f) && g_ascii_isalnum(event->keyval) || event->keyval=='_' ) {
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
	*/

	return TRUE;
}

static void do_button_release(LanguageTips* self, GtkTextView* view, GtkTextBuffer* buf, GdkEventButton* event, CppFile* file) {
	GtkTextIter it;
	GtkTextIter end;
	gunichar ch;
	size_t line;

    // tag test
	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
	if( is_in_comment(&it) )
		return;

    // include test
	if( event->state & GDK_CONTROL_MASK ) {
		gchar* str;
		GMatchInfo* inc_info;
		gboolean is_include_line;
		gchar* inc_info_text;
		GtkTextIter ps = it;
		GtkTextIter pe = it;

		gtk_text_iter_set_line_offset(&ps, 0);
		if( !gtk_text_iter_ends_line(&pe) )
			gtk_text_iter_forward_to_line_end(&pe);

		str = gtk_text_iter_get_text(&ps, &pe);
		inc_info = 0;
		is_include_line = g_regex_match(self->re_include, str, (GRegexMatchFlags)0, &inc_info);

		if( is_include_line ) {
			GMatchInfo* info = 0;
			inc_info_text = g_match_info_fetch(inc_info, 1);
			if( g_regex_match(self->re_include_info, inc_info_text, (GRegexMatchFlags)0, &info) ) {
				gchar* sign = g_match_info_fetch(info, 1);
				gchar* filename = g_match_info_fetch(info, 2);
				if( file ) {
					open_include_file(self, filename, *sign=='<', file->filename->buf);
				} else {
					GString* filepath = self->app->doc_get_url(buf);
					if( filepath )
						open_include_file(self, filename, *sign=='<', filepath->str);
				}
				g_free(sign);
				g_free(filename);
			}
			g_free(inc_info_text);
			g_match_info_free(info);
		}
		g_match_info_free(inc_info);
		g_free(str);

		if( is_include_line )
			return;
	}

	if( !file )
		return;

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
	line = (size_t)gtk_text_iter_get_line(&it) + 1;

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
		/*
		DocIter_Gtk ps(&it);
		DocIter_Gtk pe(&end);
		std::string key;
		if( !find_key(key, ps, pe, false) )
			return;

		gchar* key_text = gtk_text_iter_get_text(&it, &end);
		preview_page_preview(preview_page, key.c_str(), key_text, *file, line);
		g_free(key_text);
		*/
	}
}

static gboolean view_on_button_release(GtkTextView* view, GdkEventButton* event, LanguageTips* self) {
	GtkTextBuffer* buf;
	GString* url;
	CppFile* file;
	// tips_hide_all(self->tips);

	buf = gtk_text_view_get_buffer(view);
	url = self->app->doc_get_url(buf);
	if( url ) {
		file = cpp_guide_find_parsed(self->cpp_guide, url->str, url->len);
		do_button_release(self, view, buf, event, file);
		if( file )
			cpp_file_unref(file);
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
	gchar* filepath = 0;
	GString* url;

	url = self->app->doc_get_url(buf);
	if( url )
		filepath = g_strndup(url->str, url->len);

	if( filepath )
		g_async_queue_push(self->parse_queue, filepath);
}

static void signals_connect(LanguageTips* self, GtkTextView* view) {
	gchar* filepath;
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

	filepath = g_strndup(url->str, url->len);
	if( filepath )
		g_async_queue_push(g_self->parse_queue, filepath);
}

static void signals_disconnect(LanguageTips* self, GtkTextView* view) {
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

    g_signal_handlers_disconnect_matched( view, (GSignalMatchType)(G_SIGNAL_MATCH_DATA), 0, 0, NULL, NULL, g_self );
    g_signal_handlers_disconnect_matched( buf, (GSignalMatchType)(G_SIGNAL_MATCH_DATA), 0, 0, NULL, NULL, g_self );
}

static void on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LanguageTips* self) {
	GtkTextView* view = self->app->doc_get_view_from_page(page);
	if( view )
		signals_connect(self, view);
}

static void on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LanguageTips* self) {
}


PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	GtkNotebook* doc_panel;
	gint i;
	gint page_count;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	g_self = g_new0(LanguageTips, 1);
	g_self->app = app;

	g_self->cpp_guide = cpp_guide_new(TRUE);

	g_self->parse_queue = g_async_queue_new_full(g_free);
	g_self->parse_thread = g_thread_create(tips_parse_thread, g_self->parse_queue, TRUE, 0);
	if( g_self->parse_thread )
		g_async_queue_ref(g_self->parse_queue);

	g_self->re_include = g_regex_new("^[ \t]*#[ \t]*include[ \t]*(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
	g_self->re_include_tip = g_regex_new("([\"<])(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
	g_self-> re_include_info = g_regex_new("([\"<])([^\">]*)[\">].*", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

	create_ui(g_self);

	doc_panel = puss_get_doc_panel(app);
	g_self->page_added_handler_id = g_signal_connect(doc_panel, "page-added",   G_CALLBACK(on_doc_page_added), g_self);
	g_self->page_removed_handler_id = g_signal_connect(doc_panel, "page-removed", G_CALLBACK(on_doc_page_removed), g_self);

	page_count = gtk_notebook_get_n_pages(doc_panel);
	for( i=0; i<page_count; ++i )
		signals_connect( g_self, app->doc_get_view_from_page_num(i) );

	g_self->update_timer = g_timeout_add(500, (GSourceFunc)on_update_timeout, g_self);

	return g_self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	if( !g_self )
		return;

	g_source_remove(g_self->update_timer);

	destroy_ui(g_self);

	if( g_self->parse_queue ) {
		g_async_queue_push(g_self->parse_queue, PARSE_THREAD_EXIT_SIGN);
		g_async_queue_unref(g_self->parse_queue);
	}

	if( g_self->parse_thread ) {
		g_thread_join(g_self->parse_thread);
	}

	cpp_guide_free(g_self->cpp_guide);

	g_free(g_self);
	g_self = 0;
}


