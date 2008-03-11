// LJCS.cpp
// 

#include "LJCS.h"

#include "PreviewPage.h"

GRegex* re_include = g_regex_new("^[ \t]*#[ \t]*include[ \t]*(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
GRegex* re_include_tip = g_regex_new("([\"<])(.*)", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);
GRegex* re_include_info = g_regex_new("([\"<])([^\">]*)[\">].*", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

LJCS::LJCS() : app(0) {}

LJCS::~LJCS() { destroy(); }

bool LJCS::create(Puss* _app) {
	app = _app;

	icons.create(app);

	preview_page = preview_page_create(app, &env);

	GtkNotebook* doc_panel = puss_get_doc_panel(app);
	g_signal_connect(doc_panel, "page-added",   G_CALLBACK(&LJCS::on_doc_page_added),   this);
	g_signal_connect(doc_panel, "page-removed", G_CALLBACK(&LJCS::on_doc_page_removed), this);

	parse_thread.run(&env);

	return true;
}

void LJCS::destroy() {
	parse_thread.stop();
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
				app->doc_open(elem->file.filename.c_str(), int(elem->sline - 1), 0);
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

gboolean LJCS::on_button_release_event(GtkWidget* view, GdkEventButton* event, gpointer tag) {
	LJCS* self = (LJCS*)tag;

    //tip_.hide_all_tip();

	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GString* url = self->app->doc_get_url(buf);

	cpp::File* file = self->env.find_parsed(std::string(url->str, url->len));
	self->do_button_release_event(view, buf, event, file);
	if( file )
		self->env.file_decref(file);

    return FALSE;
}

void LJCS::on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, gpointer tag) {
	LJCS* self = (LJCS*)tag;

	GtkTextView* view = self->app->doc_get_view_from_page(page);
	if( !view )
		return;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	g_signal_connect(view, "button-release-event", G_CALLBACK(&LJCS::on_button_release_event), tag);

    //cons.push_back( view.signal_key_press_event().connect(		sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_key_press_event),		&page ), false ) );
    //cons.push_back( view.signal_key_release_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_key_release_event),		&page )		   ) );
    //cons.push_back( view.signal_button_release_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_button_release_event),	&page ), false ) );
    //cons.push_back( view.signal_motion_notify_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_motion_notify_event),	&page )		   ) );
    //cons.push_back( view.signal_focus_out_event().connect(		sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_focus_out_event),		&page )		   ) );
    //cons.push_back( view.signal_scroll_event().connect(			sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_scroll_event),			&page )		   ) );
    //cons.push_back( view.get_buffer()->signal_modified_changed().connect( sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_modified_changed), &page ) ) );

	GString* url = self->app->doc_get_url(buf);
	if( url )
		self->parse_thread.add(std::string(url->str, url->len));
}

void LJCS::on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, gpointer tag) {
}

