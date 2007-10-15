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
	void hide_all_tip();

	Gtk::Window& list_window() const { return *list_window_; }
	Gtk::Window& decl_window() const { return *decl_window_; }

	bool locate_sub(int x, int y, Glib::ustring key);

    cpp::Element* get_selected();

    void select_next();

    void select_prev();

private:
    void create();
    void destroy();

	Glib::RefPtr<Gdk::Pixbuf> get_icon_from_elem(cpp::Element& elem);

private:
	typedef std::map<std::string, cpp::Element*> ElementMap;

	void clear_list_store();

	void fill_list_store(ElementMap& emap);

private:
	LJEditor&						editor_;

	Gtk::Window*					decl_window_;
	gtksourceview::SourceView*		decl_view_;

	Gtk::Window*					list_window_;
    Gtk::TreeView					list_view_;
    Glib::RefPtr<Gtk::ListStore>	list_store_;

    struct ModelColumns : public Gtk::TreeModelColumnRecord {
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >	icon;
        Gtk::TreeModelColumn<Glib::ustring>					name;
        Gtk::TreeModelColumn<cpp::Element*>					elem;

        ModelColumns() { add(icon); add(name); add(elem); }
    };

    const ModelColumns columns_;
};

#endif//LJED_INC_TIPWINDOW_H

