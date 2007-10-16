// parser.h
// 

#ifndef LJCS_PARSER_H
#define LJCS_PARSER_H

#include "ds.h"

// TODO : ParserEnviron改为多线程版本实现
// 

cpp::File* ljcs_parse(const std::string& filepath, bool* stopsign=0);
cpp::File* ljcs_parse_in_include_path(const std::string& filename, bool* stopsign=0);
cpp::File* ljcs_parse_text(const std::string& text, size_t sline=1, bool* stopsign=0);
void ljcs_parse_macro_replace(std::string& text, cpp::File* env);

class ParserEnviron {
public:
	StrVector		include_paths;		// right format : /usr/include/ or c:/mingw/include/
										// wrong format : /usr/include  or c:\mingw\include\

	StrVector		pre_parse_files;

	cpp::FileMap&	parsed_files() { return parsed_files_; }

private:
	cpp::FileMap	parsed_files_;

public:
	static ParserEnviron& self();

	bool in_include_path(const std::string& path);

	bool append_include_path(const std::string& path);

	void add_parsed(cpp::File* file);				// add and ref
	void remove_parsed(cpp::File* file);			// remove and unref
	void remove_parsed(const std::string& abspath);

	cpp::File* find_parsed(const std::string& filename);
	cpp::File* find_parsed_in_include_path(const std::string& filename);

	cpp::File* find_include_file(const cpp::Include& inc);

public:
	bool abspath_in_include_path(const std::string& abspath);
	cpp::File* abspath_find_parsed(const std::string& abspath);

private:
	ParserEnviron();
	~ParserEnviron();
	
	ParserEnviron(const ParserEnviron& o);				// no implement
	ParserEnviron& operator = (const ParserEnviron& o);	// no implement
};

#endif//LJCS_PARSER_H

