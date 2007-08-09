// SetupDialog.cpp
// 

#include "SetupDialog.h"

/*
    static MainWindowImpl* create__(const std::string& path);

	MainWindowImpl(GtkWindow* widget, Glib::RefPtr<Gnome::Glade::Xml> xml);
MainWindowImpl* MainWindowImpl::create__(const std::string& path) {
	// 
	Glib::RefPtr<Gnome::Glade::Xml> xml = Gnome::Glade::Xml::create(path + "/conf/main.glade");
	
	MainWindowImpl* mw;

	xml->get_widget_derived("MainWindow", mw);
	xml->get_widget_derived("MainWindow.DocManager", mw->doc_manager__);

	//mw->create(path);
	return mw;
}
*/

void show_setup_dialog(Gtk::Window& parent) {
	Gtk::MessageDialog dlg(parent, "ljcs setup");
	dlg.run();
}

