// TipWindow.h
// 

#ifndef LJED_INC_TIPWINDOW_H
#define LJED_INC_TIPWINDOW_H

#include "LJEditor.h"

#include <ljcs/ljcs.h>

class TipWindow : public Gtk::Window {
public:
    TipWindow(LJEditor& editor);
    virtual ~TipWindow();

    void show_tip(int x, int y, cpp::ElementSet& element_set, char tag);

	bool locate_sub(int x, int y, Glib::ustring key);

    cpp::Element* get_selected();

    char tag() const { return tag_; }

    void select_next();

    void select_prev();

private:
    void create();
    void destroy();

	Glib::RefPtr<Gdk::Pixbuf> get_icon_from_elem(cpp::Element& elem);

private:
	typedef std::map<std::string, cpp::Element*> ElementMap;

	void clear_define_store();

	void fill_define_store(ElementMap& emap);

private:
	LJEditor&						editor_;

	Gtk::Notebook					pages_;
    Gtk::TreeView					elems_view_;
	gtksourceview::SourceView*		infos_view_;

    Glib::RefPtr<Gtk::ListStore>	elems_store_;

    struct ModelColumns : public Gtk::TreeModelColumnRecord {
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >	icon;
        Gtk::TreeModelColumn<Glib::ustring>					name;
        Gtk::TreeModelColumn<cpp::Element*>					elem;

        ModelColumns() { add(icon); add(name); add(elem); }
    };

    const ModelColumns columns_;

private:
    char			tag_;
};

#endif//LJED_INC_TIPWINDOW_H

