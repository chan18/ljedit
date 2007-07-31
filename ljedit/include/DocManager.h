// DocManager.h
// 

#ifndef LJED_INC_DOCMANAGER_H
#define LJED_INC_DOCMANAGER_H

#include "gtkenv.h"

class Page {
protected:
    Page() {}
    ~Page() {}

private: // nocopyable
    Page(const Page& o);
    Page& operator=(const Page& o);

public:
    virtual const Glib::ustring& filepath() const = 0;

	Glib::RefPtr<Gtk::TextBuffer> buffer() { return view().get_buffer(); }

    virtual Gtk::TextView& view() = 0;
};

class DocManager : public Gtk::Notebook {
protected:
    DocManager() {}
    virtual ~DocManager() {}

private: // nocopyable
    DocManager(const DocManager& o);
    DocManager& operator=(const DocManager& o);

public:
	virtual Page& child_to_page(Gtk::Widget& child) = 0;

    virtual void create_new_file() = 0;
    virtual void open_file(const std::string& filepath, int line=0) = 0;
    virtual void save_current_file() = 0;
    virtual void close_current_file() = 0;
    virtual void save_all_files() = 0;
    virtual void close_all_files() = 0;

public:
    virtual Gtk::TextView* create_source_view() = 0;
    virtual void destroy_source_view(Gtk::TextView* view) = 0;

};

#endif//LJED_INC_DOCMANAGER_H

