// DocManagerImpl.h
// 

#ifndef LJED_INC_DOCMANAGERIMPL_H
#define LJED_INC_DOCMANAGERIMPL_H

#include "DocManager.h"

#include <gtksourceviewmm/sourceview.h>

class PageImpl : public Page, public Gtk::ScrolledWindow
{
public:
    static PageImpl* create(const std::string& filepath
        , const std::string& display_name
        , Glib::RefPtr<gtksourceview::SourceBuffer> buffer);

public:
    virtual bool modified() const
        { return source_buffer()->get_modified(); }

    virtual const Glib::ustring& filepath() const
        { return filepath_; }

    virtual Glib::RefPtr<Gtk::TextBuffer> buffer()
        { return source_buffer(); }

    virtual Gtk::TextView& view()
        { return source_view(); }

    Glib::ustring& filepath() { return filepath_; }

    Gtk::Label& label() { return label_; }

    Glib::RefPtr<gtksourceview::SourceBuffer> source_buffer()
        { return source_view().get_source_buffer(); }

    Glib::RefPtr<const gtksourceview::SourceBuffer> source_buffer() const
        { return source_view().get_source_buffer(); }

    gtksourceview::SourceView& source_view() { return view_; }

    const gtksourceview::SourceView& source_view() const { return view_; }

private:
    PageImpl(Gtk::Label& label, gtksourceview::SourceView& view)
        : label_(label)
        , view_(view) {}
    ~PageImpl() {}

private:
    Glib::ustring				filepath_;
    Gtk::Label&					label_;
    gtksourceview::SourceView&	view_;
};

class DocManagerImpl : public DocManager {
public:
    DocManagerImpl();
    ~DocManagerImpl();

public:
	virtual Page& child_to_page(Gtk::Widget& child)
		{ return ((PageImpl&)child); }

    virtual void create_new_file();
    virtual void open_file(const std::string& filepath, int line=0);
    virtual void save_current_file();
    virtual void close_current_file();
    virtual void save_all_files();
    virtual void close_all_files();

public:
    virtual Gtk::TextView* create_source_view();
    virtual void destroy_source_view(Gtk::TextView* view);

protected:
    bool save_page(PageImpl& page);
    bool open_page(const std::string filepath
        , const std::string& displaty_name
        , Glib::RefPtr<gtksourceview::SourceBuffer> buffer
        , int line=0 );
    bool close_page(PageImpl& page);

    void locate_page_line(int page_num, int line);

    bool scroll_to_file_pos();

private:
    Page* create_page();

private:
    void on_doc_modified_changed(PageImpl* page);

    int page_num_;
    int line_num_;
};

#endif//LJED_INC_DOCMANAGERIMPL_H

