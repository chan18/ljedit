// SetupDialog.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "SetupDialog.h"

#include <string>
#include <sstream>
#include <fstream>
#include <libglademm.h>

#include "LJCSEnv.h"


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
		, includes_(0)
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

		Glib::RefPtr<Gtk::TextBuffer> text_buffer = dlg->includes_->get_buffer();
		assert( text_buffer != 0 );

		StrVector paths = LJCSEnv::self().get_include_paths();
		StrVector::iterator it = paths.begin();
		StrVector::iterator end = paths.end();
		for( ; it!=end; ++it ) {
			text_buffer->insert_at_cursor(*it);
			text_buffer->insert_at_cursor("\n");
		}

		return dlg;
	}

	Glib::ustring get_includes() {
		assert( includes_ != 0 );
		return includes_->get_buffer()->get_text();
	}

private:
	std::string		config_filename_;

	Gtk::TextView*	includes_;
};

void load_includes(const std::string& includes) {
	StrVector paths;
			
	std::istringstream iss(includes);
	std::string line;
	while( std::getline(iss, line) ) {
		if( line.empty() )
			continue;
		paths.push_back(line);
	}

	LJCSEnv::self().set_include_paths(paths);
}

std::string get_config_filepath(const std::string& plugin_path) {
	#ifdef WIN32
		return Glib::build_filename(plugin_path, ".ljcs.conf");
	#else
		return Glib::build_filename(Glib::get_home_dir(), ".ljcs.conf");
	#endif
}

void load_setup(const std::string& plugin_path) {
	std::string text;
	std::string filename = get_config_filepath(plugin_path);

	std::ifstream ifs(filename.c_str());
	if( ifs ) {
		std::string line;
		while( std::getline(ifs, line) )
		{
			text += line;
			text += '\n';
		}

	} else {
		text =	"/usr/include/\n"
				"/usr/include/c++/4.1/\n"
				"c:/mingw32/include/\n"
				"c:/mingw32/include/c++/3.4.2/\n"
				"d:/mingw32/include/\n"
				"d:/mingw32/include/c++/3.4.2/\n";
	}

	// include path
	load_includes(text);

	// pre parse files
	StrVector ppfils;
	std::string ppfile =
#ifdef WIN32
		Glib::build_filename(plugin_path, "ljcs_pre_parse.mingw32");
#else
		Glib::build_filename(plugin_path, "ljcs_pre_parse.linux");
#endif
	ppfils.push_back(ppfile);
	LJCSEnv::self().set_pre_parse_files(ppfils);
}

void save_setup(const std::string& plugin_path, const std::string& includes) {
	std::string filename = get_config_filepath(plugin_path);
	std::ofstream ofs(filename.c_str());
	if( ofs ) {
		ofs << includes;
	}
}

void show_setup_dialog(Gtk::Window& parent, const std::string& plugin_path) {
	std::auto_ptr<SetupDialog> dlg( SetupDialog::create(plugin_path) );
	int res = dlg->run();

	switch( res ) {
	case Gtk::RESPONSE_APPLY:
	case Gtk::RESPONSE_OK:
		{
			std::string includes = dlg->get_includes();
			load_includes(includes);
			save_setup(plugin_path, includes);
		}
		break;
	default:
		break;
	}
}

