// TipWindow.h
// 

#ifndef LJED_INC_TIPWINDOW_H
#define LJED_INC_TIPWINDOW_H

#include "gtkenv.h"

#include <ljcs/ljcs.h>

class TipWindow : public Gtk::Window {
public:
    TipWindow();
    virtual ~TipWindow();

    void show_tip(int x, int y, cpp::ElementSet& element_set, char tag);

    cpp::Element* get_selected();

    char tag() const { return tag_; }

    void select_next();

    void select_prev();

private:
    void create();
    void destroy();

	Glib::RefPtr<Gdk::Pixbuf> get_icon_from_elem(cpp::Element& elem);

private:	// /window
    Gtk::VBox		vbox_;

private:	// /window/vbox
    Gtk::ScrolledWindow				scrolled_window_;
    Gtk::TreeView					defines_view_;
    Glib::RefPtr<Gtk::ListStore>	defines_store_;

    struct ModelColumns : public Gtk::TreeModelColumnRecord {
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >	icon;
        Gtk::TreeModelColumn<Glib::ustring>					name;
        Gtk::TreeModelColumn<cpp::Element*>					elem;

        ModelColumns() { add(icon); add(name); add(elem); }
    };

    const ModelColumns columns_;

private:
    cpp::ElementSet element_set_;

    char			tag_;
};

#endif//LJED_INC_TIPWINDOW_H

