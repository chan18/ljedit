// PreviewPage.h
// 

#ifndef LJED_INC_PREVIEWPAGE_H
#define LJED_INC_PREVIEWPAGE_H

#include "gtkenv.h"
#include "DocManager.h"

#include "ljcs/ljcs.h"

class PreviewPage {
public:
    Gtk::Widget& get_widget() { return vbox_; }

    void create();
    void destroy();

public:
    PreviewPage(DocManager& dm);
    virtual ~PreviewPage();

    void preview(cpp::ElementSet& mset);

private:
    bool on_scroll_to_define_line();

private:
    DocManager& dm_;

private:
    Gtk::VBox				vbox_;
    
    Gtk::Label				label_;
    Gtk::ScrolledWindow		sw_;
    
    Gtk::TextView*			view_;
    cpp::ElementSet			mset_;
};

#endif//LJED_INC_PREVIEWPAGE_H

