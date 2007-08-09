// MainWindow.h
// 

#ifndef LJED_INC_MAINWINDOW_H
#define LJED_INC_MAINWINDOW_H

#include "DocManager.h"

class MainWindow : public Gtk::Window {
protected:
    MainWindow(Gtk::WindowType type=Gtk::WINDOW_TOPLEVEL)
        : Gtk::Window(type) {}

    virtual ~MainWindow() {}

public:
    virtual Glib::RefPtr<Gtk::UIManager>	ui_manager() = 0;
    virtual Glib::RefPtr<Gtk::ActionGroup>	action_group() = 0;

    virtual Gtk::Notebook&					left_panel() = 0;
    virtual DocManager&						doc_manager() = 0;
    virtual Gtk::Notebook&					right_panel() = 0;

    virtual Gtk::Notebook&					bottom_panel() = 0;

    virtual Gtk::Statusbar&					status_bar() = 0;
};

#endif//LJED_INC_MAINWINDOW_H

