// DocPageImpl.h
// 

#ifndef LJED_INC_DOCPAGEIMPL_H
#define LJED_INC_DOCPAGEIMPL_H

#include "DocPage.h"

#include <gtksourceviewmm/sourceview.h>

class DocPageImpl : public DocPage, public Gtk::ScrolledWindow
{
public:
    static DocPageImpl* create(const std::string& filepath
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
    DocPageImpl(Gtk::Label& label, gtksourceview::SourceView& view)
        : label_(label)
        , view_(view) {}
    ~DocPageImpl() {}

private:
    Glib::ustring				filepath_;
    Gtk::Label&					label_;
    gtksourceview::SourceView&	view_;
};

#endif//LJED_INC_DOCPAGEIMPL_H

