// MiniCmdWindow.cpp
// 

#include "MiniCmdWindow.h"


MiniCmdWindow::MiniCmdWindow()
    : Gtk::Window(Gtk::WINDOW_POPUP)
{
    add(entry_);
}

MiniCmdWindow::~MiniCmdWindow() {
}

void MiniCmdWindow::show_cmd(int x, int y, Glib::ustring text) {

}

