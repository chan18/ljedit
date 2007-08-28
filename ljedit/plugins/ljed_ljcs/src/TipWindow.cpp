// TipWindow.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "TipWindow.h"

TipWindow::TipWindow(LJEditor& editor)
    : Gtk::Window(Gtk::WINDOW_POPUP)
	, editor_(editor)
{
    create();
}

TipWindow::~TipWindow() {
    destroy();
}

void TipWindow::create() {
    elems_store_ = Gtk::ListStore::create(columns_);
	elems_store_->set_sort_column(columns_.name, Gtk::SORT_ASCENDING);

    elems_view_.set_model(elems_store_);
	elems_view_.set_headers_visible(false);
	elems_view_.append_column("icon", columns_.icon);
    elems_view_.append_column("name", columns_.name);

	// view
    infos_view_ = editor_.utils().create_gtk_source_view();
    if( infos_view_ == 0 )
		return;

	Glib::RefPtr<gtksourceview::SourceLanguage> lang = editor_.utils().get_source_language_manager()->get_language_from_mime_type("text/x-c++hdr");
	Glib::RefPtr<gtksourceview::SourceBuffer> buffer = infos_view_->get_source_buffer();
	buffer->set_language(lang);
	buffer->set_highlight();

	infos_view_->set_editable(false);
	infos_view_->set_cursor_visible(false);

	Gdk::Color bg_color;
	bg_color.set_rgb_p(0.9, 0.9, 0.7);
	infos_view_->modify_base(Gtk::STATE_NORMAL, bg_color);

	Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
    sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	sw->add(elems_view_);

	pages_.set_show_tabs(false);
    pages_.append_page(*sw);
	pages_.append_page(*infos_view_);

	add(pages_);
    resize(200, 120);
    set_border_width(1);
	pages_.show_all();
}

void TipWindow::destroy() {
	clear_define_store();
}

Glib::RefPtr<Gdk::Pixbuf> TipWindow::get_icon_from_elem(cpp::Element& elem) {
	static Glib::RefPtr<Gdk::Pixbuf> icon = render_icon(Gtk::Stock::JUMP_TO, Gtk::ICON_SIZE_MENU);
	return icon;
}

void TipWindow::clear_define_store() {
	Gtk::TreeIter it = elems_store_->children().begin();
	Gtk::TreeIter end = elems_store_->children().end();
	for( ; it!=end; ++it ) {
		cpp::Element* elem = it->get_value(columns_.elem);
		elem->file.ref();
	}
	elems_store_->clear();
}

void TipWindow::fill_define_store(ElementMap& emap) {
	ElementMap::iterator it = emap.begin();
	ElementMap::iterator end = emap.end();
	for( size_t i=0; it!=end && i < 250; ++it ) {
		++i;
		cpp::Element* elem = it->second;
		elem->file.ref();

		Gtk::TreeRow row = *(elems_store_->append());
		row[columns_.icon] = get_icon_from_elem(*elem);
		row[columns_.name] = elem->name;
		row[columns_.elem] = elem;
	}
}

void TipWindow::show_tip(int x, int y, cpp::ElementSet& mset, char tag) {
	tag_ = tag;

	clear_define_store();

	if( mset.empty() ) {
		hide();
		return;
	}

	if( tag_=='s' ) {
		ElementMap emap;
		{
			cpp::ElementSet::iterator it = mset.begin();
			cpp::ElementSet::iterator end = mset.end();
			for( ; it!=end; ++it )
				emap[(*it)->name] = *it;
		}

		fill_define_store(emap);

		elems_view_.get_selection()->select(elems_store_->children().begin());
		elems_view_.columns_autosize();

		pages_.set_current_page(0);

	} else {
		char str[1024];
		str[0] = '\0';

		Glib::RefPtr<Gtk::TextBuffer> buf = infos_view_->get_buffer();
		buf->set_text("");
		cpp::ElementSet::iterator it = mset.begin();
		cpp::ElementSet::iterator end = mset.end();
		for( ; it!=end; ++it ) {
			cpp::Element* elem = *it;

			buf->insert_at_cursor(elem->decl);
			
			sprintf(str, "\n    // %s:%d\n\n", elem->file.filename.c_str(), elem->sline);

			buf->insert_at_cursor(str);
		}
		buf->place_cursor( buf->begin() );
		
		pages_.set_current_page(1);
	}

#ifdef WIN32
	resize(150, 150);
	pages_.resize_children();
	resize_children();
#endif

	move(x, y);
	show();
}

cpp::Element* TipWindow::get_selected() {
    cpp::Element* result = 0;
    if( !elems_store_->children().empty() ) {
        Glib::RefPtr<Gtk::TreeSelection> selection = elems_view_.get_selection();
        if( selection->get_selected_rows().size() > 0 ) {
            Gtk::TreeModel::iterator it = selection->get_selected();
            if( it!=elems_store_->children().end() )
                it->get_value(2, result);
        }
    }
    return result;
}

void TipWindow::select_next() {
    if( elems_store_->children().empty() )
        return;

    Glib::RefPtr<Gtk::TreeSelection> selection = elems_view_.get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();
    if( it!=elems_store_->children().end() ) {
        ++it;
        selection->select(it);
        elems_view_.scroll_to_row(elems_store_->get_path(it));
    }
}

void TipWindow::select_prev() {
    if( elems_store_->children().empty() )
        return;

    Glib::RefPtr<Gtk::TreeSelection> selection = elems_view_.get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();
    if( it!=elems_store_->children().begin() ) {
        --it;
        selection->select(it);
        elems_view_.scroll_to_row(elems_store_->get_path(it));
    }
}

