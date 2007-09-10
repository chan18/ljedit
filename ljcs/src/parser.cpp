// parser.cpp
// 

#include "crt_debug.h"

#include "parser.h"
#include "rmacro.h"
#include "cp.h"

#include <sstream>
#include <sys/stat.h>

#include "dir_utils.h"

//---------------------------------------------------------------
// ParserEnviron

bool ParserEnviron::in_include_path(const std::string& path) {
	std::string abspath = path;
	::ljcs_filepath_to_abspath(abspath);
	return abspath_in_include_path(abspath);
}

bool ParserEnviron::append_include_path(const std::string& path) {
	std::string abspath = path;
	::ljcs_filepath_to_abspath(abspath);
	size_t sz = abspath.size();
	if( sz > 0 ) {
		if( abspath[sz - 1]!='/' )
			abspath += '/';
		if( std::find(include_paths.begin(), include_paths.end(), path)==include_paths.end() ) {
			include_paths.push_back(abspath);
			return true;
		}
	}
	return false;
}

void ParserEnviron::add_parsed(cpp::File* file) {
	assert( file != 0 );
	remove_parsed(file);
	parsed_files_[file->filename] = file->ref();
}

void ParserEnviron::remove_parsed(cpp::File* file) {
	assert( file != 0 );
	remove_parsed(file->filename);
}

void ParserEnviron::remove_parsed(const std::string& abspath) {
	cpp::FileMap::iterator it = parsed_files_.find(abspath);
	if( it != parsed_files_.end() ) {
		it->second->unref();
		parsed_files_.erase(it);
	}
}

cpp::File* ParserEnviron::find_parsed(const std::string& filename) {
	std::string abspath = filename;
	::ljcs_filepath_to_abspath(abspath);
	return abspath_find_parsed(abspath);
}

cpp::File* ParserEnviron::find_parsed_in_include_path(const std::string& filename) {
	std::string abspath;

	cpp::File* retval = 0;
	StrVector::const_iterator it = ParserEnviron::self().include_paths.begin();
	StrVector::const_iterator end = ParserEnviron::self().include_paths.end();
	for( ; it!=end && retval==0; ++it ) {
		abspath = *it + filename;
		ljcs_filepath_to_abspath(abspath);
		retval = abspath_find_parsed(abspath);
	}
	return retval;
}

cpp::File* ParserEnviron::find_include_file(const cpp::Include& inc) {
	cpp::File* retval = 0;

	if( !inc.sys_header ) {
		std::string abspath = inc.filename;
		if( get_abspath_pos(inc.filename)==0 ) {
			size_t pos = inc.file.filename.find_last_of('/');
			if( pos!=std::string::npos )
				abspath.insert(0, inc.file.filename, 0, pos+1 );
		}

		ljcs_filepath_to_abspath(abspath);
		retval = abspath_find_parsed(abspath);
	}

	if( retval==0 )
		retval = find_parsed_in_include_path(inc.filename);

	return retval;
}

bool ParserEnviron::abspath_in_include_path(const std::string& abspath) {
	StrVector::iterator it = include_paths.begin();
	StrVector::iterator end = include_paths.end();
	while( it!=end ) {
		if( abspath.compare(0, it->size(), *it)==0 )
			return true;
		++it;
	}
	return false;
}

cpp::File* ParserEnviron::abspath_find_parsed(const std::string& abspath) {
	cpp::FileMap::iterator it = parsed_files_.find(abspath);
	return it == parsed_files_.end() ? 0 : it->second;
}

ParserEnviron::ParserEnviron() {
	// create default environment
}

ParserEnviron::~ParserEnviron() {
	cpp::FileMap::iterator it = parsed_files_.begin();
	cpp::FileMap::iterator end = parsed_files_.end();
	for( ; it != end; ++it )
		it->second->unref();
	parsed_files_.clear();
}

namespace {

class Parser : public MacroMgr {
private:
	static cpp::FileMap		parsing_files_;

public:
	Parser(ParserEnviron& env, bool& stopsign)
		: env_(env)
		, stopsign_(stopsign) {}

	~Parser() {}

	void reset() { reset_macros();	included_files_.clear(); }

	void parse_macro_replace(std::string& text, cpp::File* env);

	cpp::File* parse(const std::string& abspath, std::ostream* os=0);

	cpp::File* parse_in_include_path(const std::string& filename, std::ostream* os=0);

	cpp::File* parse_text(const std::string& text, size_t sline=1, std::ostream* os=0);

	bool is_stopsign_set() const { return stopsign_; }

private:
	virtual void on_include(cpp::Include& inc, const void* tag);

protected:
	void include_file(cpp::File* file);

protected:
	ParserEnviron&	env_;
	bool&			stopsign_;

	cpp::FileMap	included_files_;
};

//----------------------------------------------------------------
// static member functions and variants
// 

cpp::FileMap		Parser::parsing_files_;

//----------------------------------------------------------------

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

cpp::File* Parser::parse(const std::string& abspath, std::ostream* os) {
	if( is_stopsign_set() )
		return 0;

	if( abspath.empty() )
		return 0;

	// check re-include
	{
		cpp::FileMap::iterator it = included_files_.find(abspath);
		if( it != included_files_.end() )
			return it->second;
	}

	// check re-parse
	{
		cpp::FileMap::iterator it = parsing_files_.find(abspath);
		if( it != parsing_files_.end() )
			return it->second;
	}

	// check file, file modify datetime...
	struct stat filestat;
	if( stat(abspath.c_str(), &filestat)!=0 )
		return 0;

	time_t datetime = filestat.st_mtime;

	// check already parsed
	{
		cpp::File* r = ParserEnviron::self().find_parsed(abspath);
		if( r != 0 ) {
			if( r->datetime==datetime ) {
				include_file(r);
				return r;
			}

			ParserEnviron::self().remove_parsed(abspath);
		}
	}

	// need parse file
	cpp::File* retval = new cpp::File(abspath);
	if( retval==0 )
		return 0;
	parsing_files_[abspath] = retval;

	// create lexer for parse
	Lexer* lexer = Lexer::create(*this, *retval, os);
	if( lexer==0 ) {
		delete retval;
		return 0;
	}

	// add macros in <pre_parse_files>
	{
		// 从后向前读, 这样, 实际分析顺序才正确
		// 
		StrVector::reverse_iterator it = ParserEnviron::self().pre_parse_files.rbegin();
		StrVector::reverse_iterator end = ParserEnviron::self().pre_parse_files.rend();
		for( ; it!=end; ++it )
			parse(*it);
	}

	// start parse
	retval->datetime = datetime;

	cxx_parser::parse(*lexer, stopsign_);
	ParserEnviron::self().add_parsed(retval);
	included_files_[retval->filename] = retval;

	parsing_files_.erase(abspath);

	delete lexer;
	return retval;
}

cpp::File* Parser::parse_text(const std::string& text, size_t sline, std::ostream* os) {
	if( is_stopsign_set() )
		return 0;

	if( text.empty() )
		return 0;

	// need parse file
	cpp::File* retval = new cpp::File("");
	if( retval==0 )
		return 0;

	// create lexer for parse
	Lexer* lexer = Lexer::create(*this, *retval, text.c_str(), text.size(), (int)sline, os);
	if( lexer==0 ) {
		delete retval;
		return 0;
	}

	// add macros in <pre_parse_files>
	{
		// 从后向前读, 这样, 实际分析顺序才正确
		// 
		StrVector::reverse_iterator it = ParserEnviron::self().pre_parse_files.rbegin();
		StrVector::reverse_iterator end = ParserEnviron::self().pre_parse_files.rend();
		for( ; it!=end; ++it )
			parse(*it);
	}

	// start parse
	cxx_parser::parse(*lexer, stopsign_);

	delete lexer;
	return retval;	
}

cpp::File* Parser::parse_in_include_path(const std::string& filename, std::ostream* os) {
	if( is_stopsign_set() )
		return 0;

	cpp::File* retval = 0;

	// check re-parse
	if( parsing_files_.find(filename) == parsing_files_.end() ) {
		std::string abspath;
		StrVector::const_iterator it = ParserEnviron::self().include_paths.begin();
		StrVector::const_iterator end = ParserEnviron::self().include_paths.end();
		for( ; it!=end && retval==0; ++it ) {
			abspath = *it;
			abspath += filename;
			ljcs_filepath_to_abspath(abspath);
			retval = parse(abspath, os);
		}
	}

	return retval;
}

void Parser::include_file(cpp::File* file) {
	if( is_stopsign_set() )
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

void Parser::on_include(cpp::Include& inc, const void* tag) {
	if( is_stopsign_set() )
		return;

	if( inc.filename.empty() )
		return;

	cpp::File* incfile = 0;

	if( !inc.sys_header ) {
		std::string abspath = inc.filename;
		if( get_abspath_pos(inc.filename)==0 ) {
			size_t pos = inc.file.filename.find_last_of('/');
			if( pos!=std::string::npos )
				abspath.insert(0, inc.file.filename, 0, pos+1 );
		}

		ljcs_filepath_to_abspath(abspath);
		incfile = parse(abspath);
	}

	if( incfile==0 )
		incfile = parse_in_include_path(inc.filename);

	if( incfile != 0 )
		inc.include_file = incfile->filename;
}

}//anonymose namespace


cpp::File* ljcs_parse(const std::string& filepath, bool* stopsign) {
	bool adapt_stopsign = false;
	std::string abspath = filepath;
	::ljcs_filepath_to_abspath(abspath);
	return Parser(ParserEnviron::self(), stopsign==0 ? adapt_stopsign : *stopsign).parse(abspath);
}

cpp::File* ljcs_parse_in_include_path(const std::string& filename, bool* stopsign) {
	bool adapt_stopsign = false;
	return Parser(ParserEnviron::self(), stopsign==0 ? adapt_stopsign : *stopsign).parse_in_include_path(filename);
}

cpp::File* ljcs_parse_text(const std::string& text, size_t sline, bool* stopsign) {
	bool adapt_stopsign = false;
	return Parser(ParserEnviron::self(), stopsign==0 ? adapt_stopsign : *stopsign).parse_text(text, sline);
}

void ljcs_parse_macro_replace(std::string& text, cpp::File* env) {
	bool adapt_stopsign = false;
	return Parser(ParserEnviron::self(), adapt_stopsign).parse_macro_replace(text, env);
}

