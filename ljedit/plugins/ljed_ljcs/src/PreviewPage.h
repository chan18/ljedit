// PreviewPage.h
// 

#ifndef LJED_INC_PREVIEWPAGE_H
#define LJED_INC_PREVIEWPAGE_H

#include "gtkenv.h"
#include "LJEditorUtils.h"

#include "ljcs/ljcs.h"

class PreviewPage {
public:
    Gtk::Widget& get_widget() { return vbox_; }

    void create();
    void destroy();

public:
    PreviewPage(LJEditorUtils& utils);
    virtual ~PreviewPage();

    void preview(cpp::ElementSet& mset);

private:
    bool on_scroll_to_define_line();

private:
    LJEditorUtils&          utils_;

private:
    Gtk::VBox				vbox_;
    
    Gtk::Label				label_;
    Gtk::ScrolledWindow		sw_;
    
    Gtk::TextView*			view_;
    cpp::ElementSet			mset_;
};

#endif//LJED_INC_PREVIEWPAGE_H

