// main.cpp
//

#include "LJEditorImpl.h"
#include "PluginManager.h"

int main(int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);

#ifdef WIN32
    std::string appname = Glib::get_application_name();
    std::string filename = Glib::find_program_in_path(appname);
	if( !filename.empty() ) {
		std::string path = Glib::path_get_dirname(filename) + "/plugins";
	    PluginManager::self().add_plugins_path(path);
	}

#else
    std::string path = argv[0];
    path.erase(path.find_last_of('/'));
    path += "/plugins";
    PluginManager::self().add_plugins_path(path);

#endif
 
   LJEditorImpl& app = LJEditorImpl::self();
    if( app.create() ) {
        app.run();
        app.destroy();
    }

    return 0;
}

/*
#include <pcre++.h>

void test_pcrepp() {
    try {
        // PCRE

        // \w	Match a "word" character (alphanumeric plus "_")
        // \W	Match a non-"word" character
        // \s	Match a whitespace character
        // \S	Match a non-whitespace character
        // \d	Match a digit character
        // \D	Match a non-digit character
        // \pP	Match P, named property.  Use \p{Prop} for longer names.
        // \PP	Match non-P
        // \X	Match eXtended Unicode "combining character sequence",
        //     equivalent to (?:\PM\pM*)
        // \C	Match a single C char (octet) even under Unicode.
        // NOTE: breaks up characters into their UTF-8 bytes,
        // so you may end up with malformed pieces of UTF-8.
        // Unsupported in lookbehind.

        // POSIX

        // alpha       IsAlpha
        // alnum       IsAlnum
        // ascii       IsASCII
        // blank       IsSpace
        // cntrl       IsCntrl
        // digit       IsDigit        \d
        // graph       IsGraph
        // lower       IsLower
        // print       IsPrint
        // punct       IsPunct
        // space       IsSpace
        //             IsSpacePerl    \s
        // upper       IsUpper
        // word        IsWord
        // xdigit      IsXDigit

        std::string exp = "([[:alpha:]]*) (\\d*)";
        pcrepp::Pcre re(exp);
        if( re.search("abc 123") ) {
            std::string s0 = re[0];
            std::string s1 = re[1];
            std::string s2 = re[2];
            int a = 0;
        }
    } catch(pcrepp::Pcre::exception e) {
        int b = 0;
    }
}
*/

