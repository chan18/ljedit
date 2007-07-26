// MiniCmdWindow.h
// 

#ifndef LJED_INC_MINICMDWINDOW_H
#define LJED_INC_MINICMDWINDOW_H

#include "gtkenv.h"

class MiniCmdWindow : public Gtk::Window {
public:
    MiniCmdWindow();
    virtual ~MiniCmdWindow();

    void show_cmd(int x, int y, Glib::ustring text);

private:
    Gtk::Entry		entry_;
};

#endif//LJED_INC_MINICMDWINDOW_H

