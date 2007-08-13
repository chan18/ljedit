// SetupDialog.cpp
// 

#include "SetupDialog.h"

#include <libglademm.h>

#include "ljcs/parser.h"


class LJCSSetupException : public Glib::Exception
{
public:
    explicit LJCSSetupException(const Glib::ustring& msg) : msg_(msg) {}
    virtual ~LJCSSetupException() throw() {}

    LJCSSetupException(const LJCSSetupException& o) : msg_(o.msg_) {}
    LJCSSetupException& operator=(const LJCSSetupException& o) { msg_ = o.msg_; }

    virtual Glib::ustring what() const { return msg_; }

private:
    Glib::ustring msg_;
};


class SetupDialog : public Gtk::Dialog {
public:
	SetupDialog(GtkDialog* castitem, Glib::RefPtr<Gnome::Glade::Xml> xml)
		: Gtk::Dialog(castitem)
		, includes_list_(0)
	{
	}

	~SetupDialog() {}

	static SetupDialog* create(const std::string& plugin_path)
	{
		// create from glade file
		Glib::RefPtr<Gnome::Glade::Xml> xml;
		xml = Gnome::Glade::Xml::create( Glib::build_filename(plugin_path, "ljcs.glade") );

		SetupDialog* dlg = 0;
		xml->get_widget_derived("SetupDialog", dlg);
		if( dlg==0 )
			throw LJCSSetupException("not find or bad setup GLADE file!");

		xml->get_widget("SetupDialog.Includes", dlg->includes_);
		if( dlg->includes_==0 )
			throw LJCSSetupException("bad setup GLADE file!");

		Glib::build_filename(plugin_path, "ljcs.conf")
	}

private:
	std::string		config_filename_;

	Gtk::TextView*	includes_;
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

	dlg->set_config_file( Glib::build_filename(plugin_path, "ljcs.conf") );
	int res = dlg->run();
	delete dlg;
	switch( res ) {
	case Gtk::RESPONSE_APPLY:
	case Gtk::RESPONSE_OK:
		{
			//ParserEnviron::self().include_paths.clear();
			//ParserEnviron::self().append_include_path(path);

			ParserEnviron::self().append_include_path("/usr/include/");
			ParserEnviron::self().append_include_path("/usr/include/c++/4.1/");
			ParserEnviron::self().append_include_path("d:/mingw32/include/");
			ParserEnviron::self().append_include_path("d:/mingw32/include/c++/3.4.2/");
		}
		break;
	default:
		break;
	}
}

