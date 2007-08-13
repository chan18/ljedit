// SetupDialog.cpp
// 

#include "SetupDialog.h"

#include <string>
#include <sstream>
#include <fstream>
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

		StrVector::iterator it = ParserEnviron::self().include_paths.begin();
		StrVector::iterator end = ParserEnviron::self().include_paths.end();
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
	ParserEnviron::self().include_paths.clear();
			
	std::istringstream iss(includes);
	std::string line;
	while( std::getline(iss, line) ) {
		if( line.empty() )
			continue;
		ParserEnviron::self().append_include_path(line);
	}
}

void load_setup(const std::string& plugin_path) {
	std::string text;

	std::string filename = Glib::build_filename(plugin_path, "ljcs.conf");
	std::ifstream ifs(filename.c_str());
	if( ifs ) {
		std::string line;
		while( std::getline(ifs, line) )
			text += line;

	} else {
		text =	"/usr/include/\n"
				"/usr/include/c++/4.1/\n"
				"c:/mingw32/include/\n"
				"c:/mingw32/include/c++/3.4.2/\n"
				"d:/mingw32/include/\n"
				"d:/mingw32/include/c++/3.4.2/\n";
	}

	load_includes(text);
}

void save_setup(const std::string& plugin_path, const std::string& includes) {
	std::string filename = Glib::build_filename(plugin_path, "ljcs.conf").c_str();
	std::ofstream ofs(filename.c_str());
	if( ofs ) {
		ofs << includes;
	}
}

void show_setup_dialog(Gtk::Window& parent, const std::string& plugin_path) {
	SetupDialog* dlg = SetupDialog::create(plugin_path);
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

	delete dlg;
}

