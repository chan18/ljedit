// DocPageImpl.h
// 

#ifndef LJED_INC_DOCPAGEIMPL_H
#define LJED_INC_DOCPAGEIMPL_H

#include "DocPage.h"

class DocPageImpl : public DocPage, public Gtk::ScrolledWindow
{
public:
    static DocPageImpl* create(const std::string& filepath
        , const Glib::ustring& display_name);

public:
    virtual bool modified() const
        { return source_buffer()->get_modified(); }

    virtual const Glib::ustring& filepath() const
        { return filepath_; }

    virtual gtksourceview::SourceView& view()
        { return source_view(); }

    Glib::ustring& filepath()			{ return filepath_; }

    Gtk::Label& label()					{ return label_; }
	Gtk::EventBox& label_event_box()	{ return label_event_box_; }

    Glib::RefPtr<gtksourceview::SourceBuffer> source_buffer()
        { return source_view().get_source_buffer(); }

    Glib::RefPtr<const gtksourceview::SourceBuffer> source_buffer() const
        { return source_view().get_source_buffer(); }

    gtksourceview::SourceView& source_view() { return view_; }

    const gtksourceview::SourceView& source_view() const { return view_; }

private:
	DocPageImpl( Gtk::Label& label
		, Gtk::EventBox& label_event_box
		, gtksourceview::SourceView& view)
			: label_(label)
			, label_event_box_(label_event_box)
			, view_(view) {}

    ~DocPageImpl() {}

private:
	bool mouse_double_click(GdkEventButton* event);

private:
    Glib::ustring				filepath_;
    Gtk::Label&					label_;
	Gtk::EventBox&				label_event_box_;
    gtksourceview::SourceView&	view_;
};

#endif//LJED_INC_DOCPAGEIMPL_H

