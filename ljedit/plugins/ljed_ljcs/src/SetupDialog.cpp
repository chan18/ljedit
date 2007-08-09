// SetupDialog.cpp
// 

#include "SetupDialog.h"

#include <libglademm.h>


class SetupDialog : public Gtk::Dialog {
public:
	SetupDialog(GtkDialog* castitem, Glib::RefPtr<Gnome::Glade::Xml> xml)
		: Gtk::Dialog(castitem)
	{
	}

	~SetupDialog() {}

private:

};

void show_setup_dialog(Gtk::Window& parent, const std::string& plugin_path) {
	Glib::RefPtr<Gnome::Glade::Xml> xml;
	try {
		xml = Gnome::Glade::Xml::create( Glib::build_filename(plugin_path, "ljcs.glade") );
		
	} catch(Gnome::Glade::XmlError e) {
		Gtk::MessageDialog err_dlg(parent, "load ljcs setup dialog GLADE file failed!");
		err_dlg.run();
		return;
	}

	SetupDialog* dlg = 0;
	if( xml->get_widget_derived("SetupDialog", dlg)==0 ) {
		Gtk::MessageDialog err_dlg(parent, "load setup GLADE file failed!");
		err_dlg.run();
		return;
	}

	dlg->run();
}

