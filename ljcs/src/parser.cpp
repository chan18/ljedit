// parser.cpp
// 

#include "crt_debug.h"

#include "parser.h"
#include "rmacro.h"
#include "cp.h"

#include <sstream>


namespace {

class Parser : public LJCSParser, public MacroMgr {
public:
	Parser(IParserEnviron& env) : LJCSParser(env) {}

	~Parser() {}

	//void reset() { reset_macros();	included_files_.clear(); }

	//void parse_macro_replace(std::string& text, cpp::File* env);

	//cpp::File* parse(const std::string& abspath, std::ostream* os=0);

	//cpp::File* parse_in_include_path(const std::string& filename, std::ostream* os=0);

	//cpp::File* parse_text(const std::string& text, size_t sline=1, std::ostream* os=0);

private:
	void include_file(cpp::File* file);

	cpp::File* do_parse(const std::string& filename, std::ostream* os=0);

	cpp::File* do_parse_in_include_path(const std::string& filename, std::ostream* os=0);

private:
	virtual void on_include(cpp::Include& inc, const void* tag);

private:
	virtual cpp::File* ljcs_do_parse(const std::string& filename, std::ostream* os)
		{ return do_parse(filename, os); }

private:
	cpp::FileMap	parsing_files_;
};

void Parser::on_include(cpp::Include& inc, const void* tag) {
	std::ostream* os = (std::ostream*)tag;

	if( stopsign_is_set() )
		return;

	if( inc.filename.empty() )
		return;

	cpp::File* incfile = 0;

	if( !inc.sys_header ) {
		std::string filekey;
		if( env_.pe_is_abspath(inc.filename) ) {
			filekey = inc.filename;
			env_.pe_format_filekey(filekey);

		} else {
			std::string path;
			env_.pe_get_path(path, inc.file.filename);
			env_.pe_build_filekey(filekey, path, inc.filename);
		}

		incfile = do_parse(filekey, os);
	}

	if( incfile==0 )
		incfile = do_parse_in_include_path(inc.filename, os);

	if( incfile != 0 )
		inc.include_file = incfile->filename;
}

cpp::File* Parser::do_parse(const std::string& filekey, std::ostream* os) {
	if( stopsign_is_set() )
		return 0;

	if( filekey.empty() )
		return 0;

	// check re-parsing
	{
		cpp::FileMap::iterator it = parsing_files_.find(filekey);
		if( it != parsing_files_.end() )
			return it->second;
	}

	// check already parsed and get file modify time
	// 
	time_t mtime = 0;
	cpp::File* retval = env_.pe_check_parsed(filekey, mtime);
	if( retval != 0 )
		return retval;

	// !!! CRUSH, but why ?
	// 
	// test on vs 2005
	// 
	//
	//std::istream* ins = env_.pe_istream_create(filekey);
	//if( ins != 0 ) {
	//	env_.pe_istream_destroy(ins);
	//}

	std::stringstream ss;
	if( env_.pe_get_content(filekey, ss) ) {
		// need parse file

		retval = new cpp::File(filekey);
		retval->datetime = mtime;

		parsing_files_[filekey] = retval;

		// create lexer for parse
		Lexer* lexer = Lexer::create(*this, *retval, ss, os, os);
		if( lexer==0 ) {
			delete retval;
			retval = 0;

		} else {
			// start parse
			retval->ref();
			cxx_parser::parse(*lexer, stopsign_);
			env_.pe_on_parsed(retval);
			delete lexer;
		}

		parsing_files_.erase(filekey);
	}

	// BUG !!!
	return retval;
}

cpp::File* Parser::do_parse_in_include_path(const std::string& filename, std::ostream* os) {
	cpp::File* retval = 0;
	std::string filekey;

	StrVector::const_iterator it = include_paths_.begin();
	StrVector::const_iterator end = include_paths_.end();
	for( ; it!=end && retval==0; ++it ) {
		// build file key
		env_.pe_build_filekey(filekey, *it, filename);

		retval = do_parse(filekey, os);
	}

	return retval;
}

/*
void Parser::parse_macro_replace(std::string& text, cpp::File* env) {
	if( text.empty() || env==0 )
		return;

	include_file(env);

	cpp::File tfile("");

	std::ostringstream oss;

	// create lexer
	Lexer* lexer = Lexer::create(*this, tfile, text.c_str(), text.size(), 0, 0);
	if( lexer!=0 ) {
		while( lexer->next_token() != 0 )
			oss << lexer->token().word;
		text = oss.str();
		delete lexer;
	}
}

void Parser::include_file(cpp::File* file) {
	if( stopsign_is_set() )
		return;

	if( file==0 )
		return;

	if( included_files_.find(file->filename) != included_files_.end() )
		return;

	included_files_[file->filename] = file;
	
	// append macros into current lexer
	cpp::Elements::iterator it = file->scope.elems.begin();
	cpp::Elements::iterator end = file->scope.elems.end();
	while( it!=end ) {
		switch( (*it)->type ) {
		case cpp::ET_MACRO:
			macro_insert((cpp::Macro*)*it);
			break;
			
		case cpp::ET_UNDEF:
			macro_remove((*it)->name);
			break;
			
		case cpp::ET_INCLUDE: {
				cpp::Include* inc = (cpp::Include*)(*it);
				assert( inc != 0 );

				include_file( env_.find_include_file(*inc) );
			}
			break;
		}
		++it;
	}
}
*/

}//anonymose namespace

LJCSParser* LJCSParser::create(IParserEnviron& env) {
	return new Parser(env);
}

