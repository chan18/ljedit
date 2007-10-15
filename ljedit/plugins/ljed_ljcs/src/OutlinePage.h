// OutlinePage.h
// 

#ifndef LJED_INC_OUTLINEPAGE_H
#define LJED_INC_OUTLINEPAGE_H

#include "gtkenv.h"
#include "ljcs/ljcs.h"

class OutlinePage {
public:
    Gtk::Widget& get_widget()	{ return vbox_; }

    void create();

public:
    OutlinePage();
    virtual ~OutlinePage();

public:
    void set_file(cpp::File* file, int line);

public:
    sigc::signal<void, const cpp::Element&>	signal_elem_actived;

private:
    void add_columns();
    void view_add_elem(const cpp::Element* elem, const Gtk::TreeNodeChildren& parent);
    void view_locate_line(size_t line, const Gtk::TreeNodeChildren& parent);
    void on_elem_clicked(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*);

private:
    Gtk::VBox						vbox_;
    Gtk::TreeView					view_;
    Gtk::ScrolledWindow				sw_;

    Glib::RefPtr<Gtk::TreeStore>	store_;
    cpp::File*						file_;
    int								line_;

private:
    struct ModelColumns : public Gtk::TreeModelColumnRecord
    {
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >	icon;
        Gtk::TreeModelColumn<Glib::ustring>					decl;
        Gtk::TreeModelColumn<const cpp::Element*>			elem;

        ModelColumns() { add(icon); add(decl); add(elem); }
    };

  const ModelColumns columns_;
};

#endif//LJED_INC_OUTLINEPAGE_H

