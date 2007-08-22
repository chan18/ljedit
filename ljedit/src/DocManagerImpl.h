// DocManagerImpl.h
// 

#ifndef LJED_INC_DOCMANAGERIMPL_H
#define LJED_INC_DOCMANAGERIMPL_H

#include "DocManager.h"
#include "DocPageImpl.h"


class DocManagerImpl : public DocManager {
public:
    DocManagerImpl();
    ~DocManagerImpl();

public:
	virtual DocPage& child_to_page(Gtk::Widget& child)
		{ return ((DocPageImpl&)child); }

    virtual void create_new_file();
    virtual void open_file(const std::string& filepath, int line=0);
    virtual void save_current_file();
    virtual void close_current_file();
    virtual void save_all_files();
    virtual void close_all_files();

protected:
    bool save_page(DocPageImpl& page);
    bool open_page(const std::string filepath
        , const std::string& displaty_name
        , const Glib::ustring* text = 0
        , int line=0 );
    bool close_page(DocPageImpl& page);

    void locate_page_line(int page_num, int line);

    bool scroll_to_file_pos();

private:
    void on_doc_modified_changed(DocPageImpl* page);
	void on_page_close_button_clicked(DocPageImpl* page);

private:
    int page_num_;
    int line_num_;
};

#endif//LJED_INC_DOCMANAGERIMPL_H

