// PreviewPage.cpp
// 

#include "PreviewPage.h"

PreviewPage::PreviewPage(LJEditorUtils& utils)
    : utils_(utils)
    , view_(0)
{
}

PreviewPage::~PreviewPage() {
}

void PreviewPage::create() {
    view_ = utils_.create_source_view();
    if( view_ == 0 )
        return;

    label_.set_alignment(0.0, 0.5);
    vbox_.pack_start(label_, false, true);

    Glib::RefPtr<Gtk::TextBuffer> buffer = view_->get_buffer();
    view_->set_buffer(buffer);
    view_->set_wrap_mode(Gtk::WRAP_NONE);
    view_->set_editable(false);

    sw_.add(*view_);
    sw_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    vbox_.pack_start(sw_);
    vbox_.show_all();
}

void PreviewPage::destroy() {
    utils_.destroy_source_view(view_);
    view_ = 0;
}

void PreviewPage::preview(cpp::ElementSet& mset) {
    cpp::unref_all_elems(mset_);
    cpp::ref_all_elems(mset);
    mset_.swap(mset);

    Glib::RefPtr<Gtk::TextBuffer> buffer = view_->get_buffer();
    Glib::ustring text;
    if( !mset_.empty() ) {
        cpp::Element* elem = *mset_.begin();
        if( elem->file.filename != label_.get_text() ) {
            try {
                Glib::RefPtr<Glib::IOChannel> ifs = Glib::IOChannel::create_from_file(elem->file.filename, "r");
                ifs->read_to_end(text);

            } catch(Glib::FileError error) {
            }

            label_.set_text(elem->file.filename);

            buffer->set_text(text);
        }

    } else {
        label_.set_text("");
        buffer->set_text(text);
    }

    Glib::signal_idle().connect( sigc::mem_fun(this, &PreviewPage::on_scroll_to_define_line) );
}

bool PreviewPage::on_scroll_to_define_line() {
    if( mset_.empty() )
        return false;

    cpp::Element* elem = *mset_.begin();
    Glib::RefPtr<Gtk::TextBuffer> buffer = view_->get_buffer();

    Gtk::TextBuffer::iterator it = buffer->get_iter_at_line(int(elem->sline - 1));
    if( it != buffer->end() )
        buffer->place_cursor(it);
    view_->scroll_to_iter(it, 0.0);

    return false;
}


