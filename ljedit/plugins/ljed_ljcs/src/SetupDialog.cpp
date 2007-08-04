// SetupDialog.cpp
// 

#include "SetupDialog.h"


void show_setup_dialog(Gtk::Window& parent) {
	Gtk::MessageDialog dlg(parent, "ljcs setup");
	dlg.run();
}

