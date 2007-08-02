// DocPage.h
// 

#ifndef LJED_INC_DOCPAGE_H
#define LJED_INC_DOCPAGE_H

#include "gtkenv.h"

class DocPage {
protected:
    DocPage() {}
    ~DocPage() {}

private: // nocopyable
    DocPage(const DocPage& o);
    DocPage& operator=(const DocPage& o);

public:
    virtual const Glib::ustring& filepath() const = 0;

	Glib::RefPtr<Gtk::TextBuffer> buffer() { return view().get_buffer(); }

    virtual Gtk::TextView& view() = 0;
};

#endif//LJED_INC_DOCPAGE_H

