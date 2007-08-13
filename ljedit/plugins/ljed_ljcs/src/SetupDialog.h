// SetupDialog.h
// 

#ifndef LJED_INC_SETUPDIALOG_H
#define LJED_INC_SETUPDIALOG_H

#include "gtkenv.h"

void load_setup(const std::string& plugin_path);

void show_setup_dialog(Gtk::Window& parent, const std::string& plugin_path);

#endif//LJED_INC_SETUPDIALOG_H

