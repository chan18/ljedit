// PreviewPage.cpp
// 

#include "PreviewPage.h"
#include "LJCSPluginUtils.h"

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

	Gdk::Color bg_color;
	bg_color.set_rgb_p(0.9, 0.9, 0.7);
	view_->modify_base(Gtk::STATE_NORMAL, bg_color);

	view_->signal_button_release_event().connect( sigc::mem_fun(this, &PreviewPage::on_sourceview_button_release_event), false );

	Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
	sw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    sw->add(*view_);

	// vbox
	vbox_.pack_start(*hbox, false, true);
    vbox_.pack_start(*sw);
    vbox_.show_all();
}

void PreviewPage::destroy() {
    cpp::unref_all_elems(elems_);
	elems_.clear();
	delete view_;
    view_ = 0;
}

void PreviewPage::preview(cpp::Elements& elems, size_t index) {
	if( &elems!=&elems_ ) {
		cpp::unref_all_elems(elems_);
		elems_.swap(elems);
		cpp::ref_all_elems(elems_);
	}

    Glib::RefPtr<Gtk::TextBuffer> buffer = view_->get_buffer();
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
			if( editor_.utils().load_file(text, elem->file.filename) )
				buffer->set_text(text);
        }

    } else {
        filename_label_->set_text("");
		number_button_->set_label("0/0");
        buffer->set_text(text);

		last_preview_file_ = 0;
    }

    Glib::signal_idle().connect( sigc::mem_fun(this, &PreviewPage::on_scroll_to_define_line) );
}

void PreviewPage::on_number_btn_clicked() {
	preview(elems_, index_ + 1);
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
    view_->scroll_to_iter(it, 0.1);

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
    Gtk::TextBuffer::iterator end = it;
    char ch = (char)it.get_char();
    if( ::isalnum(ch) || ch=='_' ) {
        while( it.forward_word_end() ) {
            ch = it.get_char();
            if( ch=='_' )
                continue;
            break;
        }
    }

    StrVector keys;
	if( !find_keys(keys, it, end, &(elem->file)) )
        return false;

    size_t line = (size_t)it.get_line() + 1;
    MatchedSet mset;
    search_keys(keys, mset, elem->file, line);

	if( !mset.elems.empty() ) {
		cpp::unref_all_elems(elems_);
		elems_.resize(mset.elems.size());
		std::copy(mset.elems.begin(), mset.elems.end(), elems_.begin());
		cpp::ref_all_elems(elems_);

		preview(elems_);
	}

    return false;
}

