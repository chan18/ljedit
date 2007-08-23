// DocManager.h
// 

#ifndef LJED_INC_DOCMANAGER_H
#define LJED_INC_DOCMANAGER_H

#include "DocPage.h"

class DocManager : public Gtk::Notebook {
protected:
    DocManager() {}

    virtual ~DocManager() {}

private: // nocopyable
    DocManager(const DocManager& o);
    DocManager& operator=(const DocManager& o);

public:
	DocPage* get_current_document() {
		Gtk::Widget* widget = get_current()->get_child();
		if( widget==0 )
			return 0;
		return &child_to_page(*widget);
	}

public:
	virtual DocPage& child_to_page(Gtk::Widget& child) = 0;

    virtual void create_new_file() = 0;
    virtual void open_file(const std::string& filepath, int line=0) = 0;
    virtual void save_current_file() = 0;
    virtual void close_current_file() = 0;
    virtual void save_all_files() = 0;
    virtual void close_all_files() = 0;
};

#endif//LJED_INC_DOCMANAGER_H

