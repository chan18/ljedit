// TipWindow.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "TipWindow.h"
#include "LJCSIcons.h"


TipWindow::TipWindow(LJEditor& editor)
    : editor_(editor)
	, decl_window_(0)
	, decl_view_(0)
	, list_window_(0)
{
    create();
}

TipWindow::~TipWindow() {
    destroy();
}

void TipWindow::create() {
	// create declare view
	{
		decl_view_ = editor_.utils().create_gtk_source_view();
		if( decl_view_ == 0 )
			return;

		Glib::RefPtr<gtksourceview::SourceLanguage> lang = editor_.utils().get_language_by_filename("a.cpp");
		Glib::RefPtr<gtksourceview::SourceBuffer> buffer = decl_view_->get_source_buffer();
		buffer->set_language(lang);
		buffer->set_highlight_syntax();

		decl_view_->set_editable(false);
		decl_view_->set_cursor_visible(false);

		Gdk::Color bg_color;
		bg_color.set_rgb_p(0.96, 0.96, 1.0);
		decl_view_->modify_base(Gtk::STATE_NORMAL, bg_color);

		Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
		sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
		sw->add(*decl_view_);

		decl_window_ = new Gtk::Window(Gtk::WINDOW_POPUP);
		if( decl_window_ == 0 )
			return;
		decl_window_->add(*sw);
		decl_window_->show_all_children();
		decl_window_->set_border_width(1);
	}

	// create list view
	{
		list_store_ = Gtk::ListStore::create(list_columns_);
		list_store_->set_sort_column(list_columns_.name, Gtk::SORT_ASCENDING);

		list_view_.set_model(list_store_);
		list_view_.set_headers_visible(false);
		list_view_.append_column("icon", list_columns_.icon);
		list_view_.append_column("name", list_columns_.name);

		Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
		sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
		sw->add(list_view_);

		list_window_ = new Gtk::Window(Gtk::WINDOW_POPUP);
		if( list_window_ == 0 )
			return;
		list_window_->add(*sw);
		list_window_->show_all_children();
		list_window_->set_border_width(1);
	}

	// create include view
	{
		include_store_ = Gtk::ListStore::create(include_columns_);
		include_store_->set_sort_column(include_columns_.name, Gtk::SORT_ASCENDING);

		include_view_.set_model(include_store_);
		include_view_.set_headers_visible(false);
		include_view_.append_column("name", include_columns_.name);

		Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
		sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
		sw->add(include_view_);

		include_window_ = new Gtk::Window(Gtk::WINDOW_POPUP);
		if( include_window_ == 0 )
			return;
		include_window_->add(*sw);
		include_window_->show_all_children();
		include_window_->set_border_width(1);
	}
}

void TipWindow::destroy() {
	clear_list_store();
}

void TipWindow::clear_list_store() {
	Gtk::TreeIter it = list_store_->children().begin();
	Gtk::TreeIter end = list_store_->children().end();
	for( ; it!=end; ++it ) {
		cpp::Element* elem = it->get_value(list_columns_.elem);
		LJCSEnv::self().file_decref(&(elem->file));
	}
	list_store_->clear();
}

void TipWindow::fill_list_store(ElementMap& emap) {
	ElementMap::iterator it = emap.begin();
	ElementMap::iterator end = emap.end();
	for( size_t i=0; it!=end && i < 250; ++it ) {
		++i;
		cpp::Element* elem = it->second;
		LJCSEnv::self().file_incref(&(elem->file));

		Gtk::TreeRow row = *(list_store_->append());
		row[list_columns_.icon] = LJCSIcons::self().get_icon_from_elem(*elem);
		row[list_columns_.name] = elem->name;
		row[list_columns_.elem] = elem;
	}
}

void TipWindow::show_decl_tip(int x, int y, cpp::ElementSet& mset) {
	if( mset.empty() ) {
		decl_window_->hide();
		return;
	}

	char str[1024];
	str[0] = '\0';

	Glib::RefPtr<Gtk::TextBuffer> buf = decl_view_->get_buffer();
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

	decl_window_->move(x, y);
	decl_window_->resize(200, 100);
	decl_window_->resize_children();
	decl_window_->show();
}

void TipWindow::show_list_tip(int x, int y, cpp::ElementSet& mset) {
	clear_list_store();

	if( mset.empty() ) {
		list_window_->hide();
		return;
	}

	ElementMap emap;
	{
		cpp::ElementSet::iterator it = mset.begin();
		cpp::ElementSet::iterator end = mset.end();
		for( ; it!=end; ++it ) {
			ElementMap::iterator p = emap.find((*it)->name);
			if( p==emap.end() ) {
				emap[(*it)->name] = *it;
				continue;
			}

			if( (*it)->type==cpp::ET_CLASS )
				p->second = *it;
		}
	}

	fill_list_store(emap);

	list_view_.get_selection()->select(list_store_->children().begin());
	list_view_.columns_autosize();

	list_window_->move(x, y);
	list_window_->resize(200, 100);
	list_window_->resize_children();
	list_window_->show();
}

void TipWindow::show_include_tip(int x, int y, const std::set<std::string>& files) {
	decl_window_->hide();
	list_window_->hide();

	include_store_->clear();

	if( files.empty() ) {
		include_window_->hide();
		return;
	}

	std::set<std::string>::const_iterator it = files.begin();
	std::set<std::string>::const_iterator end = files.end();
	for( ; it!=end; ++it ) {
		Gtk::TreeRow row = *(include_store_->append());
		row[include_columns_.name] = *it;
	}

	include_view_.get_selection()->select(include_store_->children().begin());
	include_view_.columns_autosize();

	include_window_->move(x, y);
	include_window_->resize(200, 100);
	include_window_->resize_children();
	include_window_->show();
}

void TipWindow::hide_all_tip() {
	decl_window_->hide();
	list_window_->hide();
	include_window_->hide();
}

bool TipWindow::locate_sub(int x, int y, Glib::ustring key) {
	Gtk::TreeModel::iterator it = list_store_->children().begin();
	Gtk::TreeModel::iterator end = list_store_->children().end();
	for( ; it!=end; ++it ) {
		Glib::ustring name = it->get_value(list_columns_.name);
		if( name.find(key)==0 ) {
			Glib::RefPtr<Gtk::TreeSelection> selection = list_view_.get_selection();
			selection->select(it);
			list_view_.scroll_to_row(list_store_->get_path(it));
			return true;
		}
	}

	return false;
}

cpp::Element* TipWindow::get_list_selected() {
    cpp::Element* result = 0;
    if( !list_store_->children().empty() ) {
        Glib::RefPtr<Gtk::TreeSelection> selection = list_view_.get_selection();
        if( selection->get_selected_rows().size() > 0 ) {
            Gtk::TreeModel::iterator it = selection->get_selected();
            if( it!=list_store_->children().end() )
                result = it->get_value(list_columns_.elem);
        }
    }
    return result;
}

Glib::ustring TipWindow::get_include_selected() {
    Glib::ustring result;
    if( !include_store_->children().empty() ) {
        Glib::RefPtr<Gtk::TreeSelection> selection = include_view_.get_selection();
        if( selection->get_selected_rows().size() > 0 ) {
            Gtk::TreeModel::iterator it = selection->get_selected();
            if( it!=include_store_->children().end() )
                result = it->get_value(include_columns_.name);
        }
    }
    return result;

}

void TipWindow::select_next() {
	if( list_window_->is_visible() ) {
		if( list_store_->children().empty() )
			return;

		Glib::RefPtr<Gtk::TreeSelection> selection = list_view_.get_selection();
		Gtk::TreeModel::iterator it = selection->get_selected();
		if( it!=list_store_->children().end() ) {
			++it;
			selection->select(it);
			list_view_.scroll_to_row(list_store_->get_path(it));
		}

	} else if( include_window_->is_visible() ) {
		Glib::RefPtr<Gtk::TreeSelection> selection = include_view_.get_selection();
		Gtk::TreeModel::iterator it = selection->get_selected();
		if( it!=include_store_->children().end() ) {
			++it;
			selection->select(it);
			include_view_.scroll_to_row(include_store_->get_path(it));
		}
	}
}

void TipWindow::select_prev() {
	if( list_window_->is_visible() ) {
		if( list_store_->children().empty() )
			return;

		Glib::RefPtr<Gtk::TreeSelection> selection = list_view_.get_selection();
		Gtk::TreeModel::iterator it = selection->get_selected();
		if( it!=list_store_->children().begin() ) {
			--it;
			selection->select(it);
			list_view_.scroll_to_row(list_store_->get_path(it));
		}

	} else if( include_window_->is_visible() ) {
		Glib::RefPtr<Gtk::TreeSelection> selection = include_view_.get_selection();
		Gtk::TreeModel::iterator it = selection->get_selected();
		if( it!=include_store_->children().begin() ) {
			--it;
			selection->select(it);
			include_view_.scroll_to_row(include_store_->get_path(it));
		}
	}
}

