// PreviewPage.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "PreviewPage.h"

#include <sstream>

#include "LJCSPluginUtils.h"
#include "LJCSEnv.h"

PreviewPage::PreviewPage(LJEditor& editor)
    : editor_(editor)
    , view_(0)
	, index_(0)
	, last_preview_file_(0)
{
}

PreviewPage::~PreviewPage() {
}

void PreviewPage::create() {
	// hbox
	number_button_ = Gtk::manage(new Gtk::Button());
	number_button_->set_label("0/0");
	number_button_->signal_clicked().connect(sigc::mem_fun(this, &PreviewPage::on_number_btn_clicked));
	number_button_->signal_button_release_event().connect(sigc::mem_fun(this, &PreviewPage::on_number_btn_release_event), false);

	Gtk::Button* btn = Gtk::manage(new Gtk::Button());
	btn->set_relief(Gtk::RELIEF_HALF);
	btn->signal_clicked().connect(sigc::mem_fun(this, &PreviewPage::on_filename_btn_clicked));

	filename_label_ = Gtk::manage(new Gtk::Label());
	filename_label_->set_alignment(0.0, 0.5);
	filename_label_->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
	btn->add(*filename_label_);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
	hbox->set_border_width(2);
	hbox->pack_start(*number_button_, false, false);
	hbox->pack_start(*btn, true, true);

	// view
    view_ = editor_.utils().create_gtk_source_view();
    if( view_ == 0 )
		return;

	Glib::RefPtr<gtksourceview::SourceLanguage> lang = editor_.utils().get_source_language_manager()->get_language_from_mime_type("text/x-c++hdr");
	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = view_->get_source_buffer();
	buffer->set_language(lang);
	buffer->set_highlight();

	view_->set_highlight_current_line(true);
	view_->set_show_line_numbers(true);
	view_->set_editable(false);

	/*
	Gdk::Color bg_color;
	bg_color.set_rgb_p(0.9, 0.9, 0.7);
	view_->modify_base(Gtk::STATE_NORMAL, bg_color);
	*/

	view_->signal_button_release_event().connect( sigc::mem_fun(this, &PreviewPage::on_sourceview_button_release_event), false );

	Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
	sw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    sw->add(*view_);

	// vbox
	vbox_.pack_start(*hbox, false, true);
    vbox_.pack_start(*sw);
    vbox_.show_all();

	// thread
	sem_init(&search_run_sem_, 0, 0);
	pthread_mutex_init(&search_key_mutex_, 0);
	pthread_mutex_init(&search_result_mutex_, 0);

	search_stopsign_ = false;
	search_resultsign_ = false;
	pthread_create(&search_pid_, NULL, &PreviewPage::wrap_thread, this);

	// update timeout
	Glib::signal_timeout().connect( sigc::mem_fun(this, &PreviewPage::on_update_timeout), 200 );
}

void PreviewPage::destroy() {
	search_stopsign_ = true;
	sem_post(&search_run_sem_);
	pthread_join(search_pid_, 0);

	pthread_mutex_destroy(&search_key_mutex_);
	pthread_mutex_destroy(&search_result_mutex_);

    cpp::unref_all_elems(elems_);
	elems_.clear();
	delete view_;
    view_ = 0;

}

void PreviewPage::do_preview(size_t index) {
	pthread_mutex_lock(&search_result_mutex_);

	if( search_resultsign_==true ) {
		search_resultsign_ = false;
		do_preview_impl(index);

	} else if( index != index_ ) {
		do_preview_impl(index);
	}

	pthread_mutex_unlock(&search_result_mutex_);
}

void PreviewPage::do_preview_impl(size_t index) {
	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = view_->get_source_buffer();
    Glib::ustring text;

    if( !elems_.empty() ) {
		index_ = index % elems_.size();
        cpp::Element* elem = elems_[index_];

		char buf[1024];
		buf[0] = '\0';

		sprintf(buf, "%s:%d", elem->file.filename.c_str(), elem->sline);
		filename_label_->set_text(buf);

		sprintf(buf, "%d/%d", index_ + 1, elems_.size());
		number_button_->set_label(buf);

		// set text
		// 
		if( &(elem->file) != last_preview_file_ ) {
			last_preview_file_ = &(elem->file);

			Glib::ustring text;
			if( editor_.utils().load_file(text, elem->file.filename) ) {
				buffer->set_text(text);
			}
        }

    } else {
        filename_label_->set_text("");
		number_button_->set_label("0/0");
		buffer->begin_not_undoable_action();
        buffer->set_text(text);
		buffer->end_not_undoable_action();

		last_preview_file_ = 0;
    }

    Glib::signal_idle().connect( sigc::mem_fun(this, &PreviewPage::on_scroll_to_define_line) );
}

void PreviewPage::on_number_btn_clicked() {
	do_preview(index_ + 1);
}

bool PreviewPage::on_number_btn_release_event(GdkEventButton* event) {
	if( event->button!=3 )
		return false;

	// make popup menu
	Gtk::Menu::MenuList& menulist = number_menu_.items();
	menulist.clear();

	if( elems_.empty() ) {
		menulist.push_back( Gtk::Menu_Helpers::MenuElem("empty") );

	} else {
		for( size_t i=0; i<elems_.size(); ++i ) {
			std::ostringstream oss;
			oss << (char)((i==index_) ? '*' : ' ') << ' ' << elems_[i]->file.filename << ':' << elems_[i]->sline;
			menulist.push_back( Gtk::Menu_Helpers::MenuElem(oss.str(), sigc::bind(sigc::mem_fun(this, &PreviewPage::on_number_menu_selected), i)) );
		}
	}

	number_menu_.popup(event->button, event->time);
	return true;
}

void PreviewPage::on_number_menu_selected(size_t index) {
	do_preview(index);
}

void PreviewPage::on_filename_btn_clicked() {
    if( elems_.empty() )
        return;

	cpp::Element* elem = elems_[index_];
	assert( elem != 0 );

	editor_.main_window().doc_manager().open_file(elem->file.filename, (int)(elem->sline - 1));
}

bool PreviewPage::on_scroll_to_define_line() {
    if( elems_.empty() )
        return false;

	cpp::Element* elem = elems_[index_];
	assert( elem != 0 );

	Glib::RefPtr<Gtk::TextBuffer> buffer = view_->get_buffer();

    Gtk::TextBuffer::iterator it = buffer->get_iter_at_line(int(elem->sline - 1));
    if( it != buffer->end() )
        buffer->place_cursor(it);
    view_->scroll_to_iter(it, 0.25);

    return false;
}

bool PreviewPage::on_sourceview_button_release_event(GdkEventButton* event) {
    if( elems_.empty() )
        return false;

	cpp::Element* elem = elems_[index_];
	assert( elem != 0 );

    // test CTRL state
	if( (event->state & Gdk::CONTROL_MASK)== 0 )
		return false;

    // tag test
    Glib::RefPtr<Gtk::TextBuffer> buf = view_->get_buffer();
    Glib::RefPtr<Gtk::TextMark> mark = buf->get_insert();
    Gtk::TextBuffer::iterator it = buf->get_iter_at_mark(mark);
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
    size_t line = (size_t)it.get_line() + 1;

	LJEditorDocIter ps(it);
	LJEditorDocIter pe(end);
	std::string key;
	if( !find_key(key, ps, pe) )
		return false;
	
	std::string key_text = it.get_text(end);

	preview(key, key_text, elem->file, line);

    return false;
}

void PreviewPage::search_thread() {
	SearchContent	sc;
	StrVector		keys;
	MatchedSet		mset;

	while( !search_stopsign_ ) {
		sem_wait(&search_run_sem_);

		if( search_stopsign_ )
			break;

		pthread_mutex_lock(&search_key_mutex_);
		sc = search_content_;
		pthread_mutex_unlock(&search_key_mutex_);
		
		keys.clear();
		mset.elems.clear();

		keys.push_back(sc.key);

		ljcs_parse_macro_replace(sc.key_text, sc.file);
		parse_key(sc.key_text, sc.key_text);
		if( !sc.key_text.empty() && sc.key_text!=sc.key )
			keys.push_back(sc.key_text);

		::search_keys(keys, mset, LJCSEnv::self().stree, sc.file, sc.line);

		pthread_mutex_lock(&search_result_mutex_);
		cpp::unref_all_elems(elems_);
		elems_.resize(mset.elems.size());
		std::copy(mset.elems.begin(), mset.elems.end(), elems_.begin());
		cpp::ref_all_elems(elems_);

		search_resultsign_ = true;
		index_ = find_best_matched_index(elems_);
		pthread_mutex_unlock(&search_result_mutex_);
	}

	pthread_exit(0);
}

void PreviewPage::preview(const std::string& key, const std::string& key_text, cpp::File& file, size_t line) {
	pthread_mutex_lock(&search_key_mutex_);

	if( search_content_.file != 0 )
		search_content_.file->unref();

	search_content_.key = key;
	search_content_.key_text = key_text;
	search_content_.file = file.ref();
	search_content_.line = line;

	sem_post(&search_run_sem_);

	pthread_mutex_unlock(&search_key_mutex_);
}

bool PreviewPage::on_update_timeout() {
	do_preview(index_);
	return true;
}

