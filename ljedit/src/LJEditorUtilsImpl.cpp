// LJEditorUtilsImpl.cpp
//

#include "StdAfx.h"	// for vc precompile header

#include "LJEditorUtilsImpl.h"
#include "ConfigManagerImpl.h"

#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <set>

#include <gtksourceview/gtksourcelanguage.h>


#ifdef WIN32
	#include <windows.h>

	#include <direct.h>
#endif

const size_t LJCS_MAX_PATH_SIZE = 8192;

const std::string default_option_loader_charset_order = "GBK UTF-16";

inline void filepath_to_abspath(std::string& filepath) {
	if( filepath.empty() )
		return;

#ifdef WIN32
	// replace /xx/./yy with /xx/yy
	std::string local_filename = Glib::locale_from_utf8(filepath);
	char buf[LJCS_MAX_PATH_SIZE];
	if( GetFullPathNameA(local_filename.c_str(), LJCS_MAX_PATH_SIZE, buf, 0) ) {
		local_filename = buf;
		filepath = Glib::locale_to_utf8(local_filename);
	}

	std::transform(filepath.begin(), filepath.end(), filepath.begin(), tolower);

#else
	if( !Glib::path_is_absolute(filepath) )
		Glib::build_filename(Glib::get_current_dir(), filepath);

	// replace /xx/./yy with /xx/yy
	{
		for(;;) {
			size_t pos = filepath.rfind("/./");
			if( pos==filepath.npos )
				break;
			filepath.erase(pos, 2);
		}
	}

	// replace /xx/../yy with /yy
	{
		for(;;) {
			size_t pe = filepath.find("/../");
			if( pe==filepath.npos || pe==0 )
				break;

			size_t ps = filepath.rfind('/', pe-1);
			if( ps==filepath.npos )
				break;

			filepath.erase(ps, (pe+3-ps));
		}
	}
#endif
}

LJEditorUtilsImpl::LJEditorUtilsImpl() {
}

LJEditorUtilsImpl::~LJEditorUtilsImpl() {
}

void LJEditorUtilsImpl::create(const std::string& path) {
	// get language map
	//
	Glib::RefPtr<gtksourceview::SourceLanguageManager> mgr = gtksourceview::SourceLanguageManager::get_default();
	
	Glib::StringArrayHandle ids = mgr->get_language_ids();
	Glib::StringArrayHandle::iterator it = ids.begin();
	Glib::StringArrayHandle::iterator end = ids.end();
	for( ; it!=end; ++it ) {
		Glib::RefPtr<gtksourceview::SourceLanguage> lang = mgr->get_language(*it);
		if( !lang )
			continue;
		
		// TODO : use regex
		// 
		Glib::StringArrayHandle globs = gtk_source_language_get_globs(lang->gobj());
		Glib::StringArrayHandle::iterator ps = globs.begin();
		Glib::StringArrayHandle::iterator pe = globs.end();
		for( ; ps!=pe; ++ps ) {
			Glib::ustring glob = *ps;
			if( glob.size() > 2 && glob[0]=='*' && glob[1]=='.' ) {
				glob.erase(0, 2);
				language_map_[glob] = lang->get_id();
			} else {
				// TODO : now not finish
				language_map_[glob] = lang->get_id();
			}
			//printf("%s:%s\n", glob.c_str(), lang->get_id().c_str());
		}
	}

	// regist loader charset
	ConfigManagerImpl& cm = ConfigManagerImpl::self();
	cm.signal_option_changed().connect( sigc::mem_fun(this, &LJEditorUtilsImpl::on_option_changed) );

	option_set_loader_charset_order(default_option_loader_charset_order);

	cm.regist_option( "editor.loader.charset_order"
		, "text"
		, default_option_loader_charset_order
		, "file loader charset order.\nnotice : UTF-8 and locale_charset are used by default!" );

	std::string option_text;
	if( cm.get_option_value("editor.loader.charset_order", option_text) )
		option_set_loader_charset_order(option_text);
}

gtksourceview::SourceView* LJEditorUtilsImpl::create_gtk_source_view() {
	return new gtksourceview::SourceView();
}

void LJEditorUtilsImpl::destroy_gtk_source_view(gtksourceview::SourceView* view) {
	delete view;
}

const Glib::ustring& LJEditorUtilsImpl::get_language_id_by_filename(const Glib::ustring& filename) {
	static Glib::ustring default_retval = "";

	// TODO : now implement not finish

	Glib::ustring name = filename;
	{
		size_t pos = name.find_last_of("\\/.");
		if( pos != name.npos ) {
			name.erase(0, pos+1);
		}
	}

	std::map<Glib::ustring, Glib::ustring>::iterator it = language_map_.find(name);
	if( it != language_map_.end() )
		return it->second;

	return default_retval;
}

void LJEditorUtilsImpl::format_filekey(std::string& filename) {
	filepath_to_abspath(filename);
}

bool LJEditorUtilsImpl::do_load_file(Glib::ustring& contents_out, Glib::ustring& charset_out, const std::string& filename) {
	Glib::ustring buf;

	std::set<std::string>	used_charsets;

	try {
		buf = Glib::file_get_contents(filename);

	} catch(const Glib::FileError&) {
		return false;
	}

	if( buf.validate() ) {
		contents_out.swap(buf);
		charset_out = "UTF-8";
		return true;
	}

	used_charsets.insert("UTF-8");

	std::string locale_charset;
	if( !Glib::get_charset(locale_charset) )	// get locale charset, and not UTF-8
	{
		try {
			contents_out = Glib::locale_to_utf8(buf);
			charset_out = locale_charset;
			return true;

		} catch(const Glib::ConvertError&) {
		}

		used_charsets.insert(locale_charset);
	}

	std::vector<std::string>::const_iterator it = option_loader_charset_order_.begin();
	std::vector<std::string>::const_iterator end = option_loader_charset_order_.end();
	for( ; it!=end; ++it ) {
		if( used_charsets.find(*it)!=used_charsets.end() )
			continue;

		try {
			contents_out = Glib::convert(buf, "UTF-8", *it);
			charset_out = *it;
			return true;

		} catch(const Glib::ConvertError&) {
		}

		used_charsets.insert(*it);
	}

	return false;
}

void LJEditorUtilsImpl::option_set_loader_charset_order(const std::string& option_text) {
	option_loader_charset_order_.clear();

	std::istringstream iss(option_text);
	std::string charset;
	while( iss >> std::ws >> charset )
		option_loader_charset_order_.push_back(charset);
}

void LJEditorUtilsImpl::on_option_changed(const std::string& id, const std::string& value, const std::string& old) {
	if( id=="editor.loader.charset_order" )
		option_set_loader_charset_order(value);
}

