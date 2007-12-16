// TipWindow.h
// 

#ifndef LJED_INC_TIPWINDOW_H
#define LJED_INC_TIPWINDOW_H

#include "LJEditor.h"

#include <ljcs/ljcs.h>

class TipWindow {
public:
    TipWindow(LJEditor& editor);
    virtual ~TipWindow();

    void show_list_tip(int x, int y, cpp::ElementSet& mset);
    void show_decl_tip(int x, int y, cpp::ElementSet& mset);
	void show_include_tip(int x, int y, const std::set<std::string>& files);
	void hide_all_tip();

	Gtk::Window& list_window() const    { return *list_window_; }
	Gtk::Window& decl_window() const    { return *decl_window_; }
	Gtk::Window& include_window() const { return *include_window_; }

	bool locate_sub(int x, int y, Glib::ustring key);
    cpp::Element* get_list_selected();
	Glib::ustring get_include_selected();
    void select_next();
    void select_prev();

	void set_decl_view_font(Pango::FontDescription& font)
		{ decl_view_->modify_font(font); }

private:
    void create();
    void destroy();

private:
	typedef std::map<std::string, cpp::Element*> ElementMap;

	void clear_list_store();

	void fill_list_store(ElementMap& emap);

private:
    struct ListModelColumns : public Gtk::TreeModelColumnRecord {
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >	icon;
        Gtk::TreeModelColumn<Glib::ustring>					name;
        Gtk::TreeModelColumn<cpp::Element*>					elem;

        ListModelColumns() { add(icon); add(name); add(elem); }
    };

    struct IncludeModelColumns : public Gtk::TreeModelColumnRecord {
        Gtk::TreeModelColumn<Glib::ustring>					name;

        IncludeModelColumns() { add(name); }
    };

private:
	LJEditor&						editor_;

	Gtk::Window*					decl_window_;
	gtksourceview::SourceView*		decl_view_;

	Gtk::Window*					list_window_;
    Gtk::TreeView					list_view_;
    Glib::RefPtr<Gtk::ListStore>	list_store_;
    const ListModelColumns			list_columns_;

	Gtk::Window*					include_window_;
    Gtk::TreeView					include_view_;
    Glib::RefPtr<Gtk::ListStore>	include_store_;
    const IncludeModelColumns		include_columns_;
};

#endif//LJED_INC_TIPWINDOW_H

