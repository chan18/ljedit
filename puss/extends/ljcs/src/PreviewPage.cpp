// PreviewPage.cpp
// 

#include "PreviewPage.h"

#include <glib/gi18n.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <sstream>

#include "LJCS.h"

gboolean PreviewPage::create(Puss* app, Environ* env) {
	app_ = app;
	env_ = env;

	// init UI
	GtkBuilder* builder = gtk_builder_new();
	if( !builder )
		return FALSE;

	gchar* filepath = g_build_filename(app_->get_module_path(), "extends", "ljcs_res", "preview_page_ui.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(ljcs) : build ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	GError* err = 0;
	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(ljcs): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	self_panel_     = GTK_WIDGET(g_object_ref(gtk_builder_get_object(builder, "vbox")));
	filename_label_ = GTK_LABEL(g_object_ref(gtk_builder_get_object(builder, "filename_label")));
	number_button_  = GTK_BUTTON(g_object_ref(gtk_builder_get_object(builder, "number_button")));
	preview_view_   = GTK_TEXT_VIEW(g_object_ref(gtk_builder_get_object(builder, "preview_view")));
	g_assert( self_panel_ && filename_label_ && number_button_ && preview_view_ );

	GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
	GtkSourceLanguage* lang = gtk_source_language_manager_get_language(lm, "cpp");
	GtkSourceBuffer* source_buffer = gtk_source_buffer_new_with_language(lang);
	gtk_text_view_set_buffer(preview_view_, GTK_TEXT_BUFFER(source_buffer));
	gtk_source_buffer_set_max_undo_levels(source_buffer, 0);
	g_object_unref(G_OBJECT(source_buffer));

	gtk_widget_show_all(self_panel_);
	gtk_notebook_append_page(puss_get_bottom_panel(app_), self_panel_, gtk_label_new(_("Preview")));

	g_object_unref(G_OBJECT(builder));

	// init search thread
	search_stopsign_ = FALSE;
	search_thread_ = g_thread_create(&PreviewPage::search_thread_wrapper, this, TRUE, 0);

	// search result preview timer
	g_timeout_add(500, &PreviewPage::search_result_update_timeout_wrapper, this);

	return TRUE;
}

void PreviewPage::destroy() {
	if( search_thread_ ) {
		search_stopsign_ = true;
		search_run_sem_.signal();
		g_thread_join(search_thread_);
		search_thread_ = 0;
	}
}

void PreviewPage::preview(const gchar* key, const gchar* key_text, cpp::File& file, size_t line) {
	GtkNotebook* bottom_panel = puss_get_bottom_panel(app_);
	gint page_num = gtk_notebook_get_current_page(bottom_panel);
	if( page_num >= 0 && self_panel_==gtk_notebook_get_nth_page(bottom_panel, page_num) ) {
		Locker locker(search_content_);
		
		search_content_->key = key;
		search_content_->key_text = key_text;
		search_content_->file = &file;
		search_content_->line = line;

		env_->file_incref(search_content_->file);

		search_run_sem_.signal();
	}
}

void PreviewPage::search_thread() {
	MatchedSet		mset(*env_);

	StrVector		keys;
	SearchContent	sc;

	while( !search_stopsign_ ) {
		// pop search content
		{
			Locker locker(search_content_);
			search_run_sem_.wait(search_content_);

			if( search_stopsign_ )
				break;

			if( sc.file != 0 ) {
				env_->file_decref(sc.file);
				sc.file = 0;
			}
			sc = search_content_.ref();
			search_content_->file = 0;
		}

		keys.clear();
		mset.clear();

		keys.push_back(sc.key);

/*
		ljcs_parse_macro_replace(sc.key_text, sc.file);
		parse_key(sc.key_text, sc.key_text);
		if( !sc.key_text.empty() && sc.key_text!=sc.key )
			keys.push_back(sc.key_text);

		pthread_rwlock_rdlock(&LJCSEnv::self().stree_rwlock);
		::search_keys(keys, mset, LJCSEnv::self().stree, sc.file, sc.line);
		pthread_rwlock_unlock(&LJCSEnv::self().stree_rwlock);

*/
		{
			RLocker locker(env_->stree());
			::search_keys(keys, mset, env_->stree().ref(), sc.file, sc.line);
		}

		{
			WLocker locker(elems_);
			cpp::Elements& elems = elems_.ref();
			env_->file_decref_all_elems(elems);
			elems.resize(mset.size());
			std::copy(mset.begin(), mset.end(), elems.begin());
			env_->file_incref_all_elems(elems);

			search_resultsign_ = true;
			//index_ = find_best_matched_index(elems);
		}
	}
}

gpointer PreviewPage::search_thread_wrapper(gpointer self) {
	((PreviewPage*)self)->search_thread();
	return 0;
}

void PreviewPage::do_preview(size_t index) {
	if( search_resultsign_ ) {
		search_resultsign_ = FALSE;
		do_preview_impl(index);

	} else if( index != index_ ) {
		do_preview_impl(index);
	}
}

void PreviewPage::do_preview_impl(size_t index) {
	RLocker locker(elems_);

	cpp::Elements& elems = elems_.ref();
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(preview_view_);

	if( elems.empty() ) {
		gtk_label_set_text(filename_label_, "");
		gtk_button_set_label(number_button_, "0/0");
		gtk_text_buffer_set_text(buffer, "", 0);
		last_preview_file_ = 0;
		return;
	}

	index_ = index % elems.size();
    cpp::Element* elem = elems[index_];

	// set filename label
	gchar* text = g_strdup_printf("%s:%d", elem->file.filename.c_str(), elem->sline);
	gtk_label_set_text(filename_label_, text);
	g_free(text);

	// set text
	if( &(elem->file) != last_preview_file_ ) {
		last_preview_file_ = &(elem->file);

		gsize len = 0;
		if( !app_->load_file(elem->file.filename.c_str(), &text, &len, 0, 0) )
			return;

		gtk_text_buffer_set_text(buffer, text, len);
		g_free(text);
    }

	g_idle_add(&PreviewPage::scroll_to_define_line_wrapper, this);
}

void PreviewPage::do_scroll_to_define_line() {
	RLocker locker(elems_);
	cpp::Elements& elems = elems_.ref();
    if( elems.empty() )
        return;

	cpp::Element* elem = elems[index_];
	assert( elem != 0 );

	GtkTextBuffer* buffer = gtk_text_view_get_buffer(preview_view_);
	if( gtk_text_buffer_get_line_count(buffer) < (gint)elem->sline ) {
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_line(buffer, &iter, (gint)(elem->sline - 1));
		if( !gtk_text_iter_is_end(&iter) ) {
			gtk_text_buffer_place_cursor(buffer, &iter);
			gtk_text_view_scroll_to_iter(preview_view_, &iter, FALSE, FALSE, 0.25, 0.25);
		}
	}
}

gboolean PreviewPage::search_result_update_timeout_wrapper(gpointer self) {
	((PreviewPage*)self)->do_preview( ((PreviewPage*)self)->index_ );
	return TRUE;
}

gboolean PreviewPage::scroll_to_define_line_wrapper(gpointer self) {
	((PreviewPage*)self)->do_scroll_to_define_line();
    return FALSE;
}
