// TipWindow.cpp
// 

#include "TipWindow.h"

TipWindow::TipWindow()
    : Gtk::Window(Gtk::WINDOW_POPUP)
{
    create();
}

TipWindow::~TipWindow() {
    destroy();
}

void TipWindow::create() {
    defines_store_ = Gtk::ListStore::create(columns_);

    defines_view_.set_model(defines_store_);
	defines_view_.append_column("icon", columns_.icon);
    defines_view_.append_column("name", columns_.name);

    scrolled_window_.add(defines_view_);
    scrolled_window_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    vbox_.pack_start(scrolled_window_);

    this->add(vbox_);
    this->resize(200, 120);
    this->set_border_width(1);

    vbox_.show_all();
}

void TipWindow::destroy() {
}

Glib::RefPtr<Gdk::Pixbuf> TipWindow::get_icon_from_elem(cpp::Element& elem) {
	static Glib::RefPtr<Gdk::Pixbuf> icon = render_icon(Gtk::Stock::JUMP_TO, Gtk::ICON_SIZE_MENU);
	return icon;
}

#include <windows.h>

void TipWindow::show_tip(int x, int y, cpp::ElementSet& mset, char tag) {
    tag_ = tag;

    cpp::unref_all_elems(element_set_);
    cpp::ref_all_elems(mset);

    element_set_.swap(mset);

    defines_view_.unset_model();
    defines_store_->clear();

	size_t _s = GetCurrentTime();
    cpp::ElementSet::iterator it = element_set_.begin();
    cpp::ElementSet::iterator end = element_set_.end();
    for( ; it!=end; ++it ) {
        Gtk::TreeRow row = *(defines_store_->append());
        row[columns_.icon] = get_icon_from_elem(**it);
        row[columns_.name] = (*it)->name;
        row[columns_.elem] = (*it);
    }
	size_t _e = GetCurrentTime();
	printf("time : %d\n", _e-_s);
    defines_view_.set_model(defines_store_);
    defines_view_.get_selection()->select(defines_store_->children().begin());
    defines_view_.columns_autosize();

    if( element_set_.empty() ) {
        hide();

    } else {
		resize(150, 150);
        move(x, y);
        Gtk::Widget::show();
    }
}

cpp::Element* TipWindow::get_selected() {
    cpp::Element* result = 0;
    if( !defines_store_->children().empty() ) {
        Glib::RefPtr<Gtk::TreeSelection> selection = defines_view_.get_selection();
        if( selection->get_selected_rows().size() > 0 ) {
            Gtk::TreeModel::iterator it = selection->get_selected();
            if( it!=defines_store_->children().end() )
                it->get_value(2, result);
        }
    }
    return result;
}


void TipWindow::select_next() {
    if( defines_store_->children().empty() )
        return;

    Glib::RefPtr<Gtk::TreeSelection> selection = defines_view_.get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();
    if( it!=defines_store_->children().end() ) {
        ++it;
        selection->select(it);
        defines_view_.scroll_to_row(defines_store_->get_path(it));
    }
}

void TipWindow::select_prev() {
    if( defines_store_->children().empty() )
        return;

    Glib::RefPtr<Gtk::TreeSelection> selection = defines_view_.get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();
    if( it!=defines_store_->children().begin() ) {
        --it;
        selection->select(it);
        defines_view_.scroll_to_row(defines_store_->get_path(it));
    }
}

