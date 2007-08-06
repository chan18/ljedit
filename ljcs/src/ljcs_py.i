// ljcs_py.i

%module(directors="1") ljcs_py

%{

#include "ljcs/ljcs.h"
#include "parse_task.h"

%}

%include <std_common.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_pair.i>
%include <std_set.i>
%include <std_map.i>
%include <std_multimap.i>

//%apply const std::string& { std::string* };

namespace cpp {

// for parse
class Element;
class File;

typedef std::vector<Element*>					Elements;

class Element {
private:
	Element();
	virtual ~Element();

public:
	const File&				file;
	
	const unsigned char		type;
	const unsigned char		view;
	const std::string		name;
	const size_t			sline;
	const size_t			eline;
	const std::string		decl;
	
%extend {
	const cpp::Elements* subscope()	{
		switch( $self->type ) {
		case cpp::ET_NAMESPACE:
		case cpp::ET_CLASS:
		case cpp::ET_ENUM:
			return &((cpp::NCScope*)$self)->scope.elems;
		}
		return 0;
	}

}

};

class File {
private:
	File();
	~File();

public:
	const std::string		filename;
	
%extend {
	const cpp::Elements& scope() { return $self->scope.elems; }

}

};

}// namespace cpp

// parser

%{

cpp::File* parse(std::string& filepath)
	{ return ljcs_parse(filepath); }

cpp::File* parse_in_include_path(std::string& filename)
	{ return ljcs_parse_in_include_path(filename); }

cpp::File* parse_text(const std::string& text, size_t sline=1)
	{ return ljcs_parse_text(text, sline); }

std::string parse_macro_replace(const std::string& text, cpp::File* env) {
	std::string result = text;
	ljcs_parse_macro_replace(result, env);
	return result;
}

%}

cpp::File* parse(std::string& filename) ;
cpp::File* parse_in_include_path(std::string& filename);
cpp::File* parse_text(const std::string& text, size_t sline=1);
std::string parse_macro_replace(const std::string& text, cpp::File* env);

typedef std::vector<std::string>	StrVector;

class ParserEnviron {
public:
	StrVector		include_paths;
	StrVector		pre_parse_files;
	cpp::FileMap&	parsed_files();

public:
	bool append_include_path(const std::string& path);
	
	bool in_include_path(const std::string& path);

	cpp::File* find_parsed(const std::string& filename);
	
	cpp::File* find_parsed_in_include_path(const std::string& filename);

public:
	bool abspath_in_include_path(const std::string& abspath);

	cpp::File* abspath_find_parsed(const std::string& abspath);
	
private:
	ParserEnviron();
	~ParserEnviron();

	ParserEnviron(const ParserEnviron& o);				// no implement
	ParserEnviron& operator = (const ParserEnviron& o);	// no implement
};

%{

ParserEnviron& env = ParserEnviron::self();

%}

const ParserEnviron& env;

%template(StrVector)	std::vector<std::string>;

%template(Elements)		std::vector<cpp::Element*>;

// parse task

class ParseTask {
public:
	void run();
	void stop();
	void add(const std::string& filename);
	bool has_result() const;
	cpp::File* query();
};

// searcher

%feature("director") IMatched;

class IMatched {
public:
	IMatched();
	virtual ~IMatched();
public:
	virtual void on_matched(cpp::Element& elem) = 0;
};

void search(const std::string& key, IMatched& cb, cpp::File& file, size_t line=0);

void search_keys(const StrVector& keys, IMatched& cb, cpp::File& file, size_t line=0);

// skey

%feature("director") IDocIter;

class IDocIter {
public:
	IDocIter(char cur);
	virtual ~IDocIter();

protected:
	virtual char do_prev() = 0;
	virtual char do_next() = 0;
};

%{

std::string py_find_key(IDocIter& ps, IDocIter& pe) {
	std::string retval;
	find_key(retval, ps, pe);
	return retval;
}

std::string parse_key(const std::string& text) {
	std::string retval;
	parse_key(retval, text);
	return retval;
}

%}

std::string py_find_key(IDocIter& ps, IDocIter& pe);

std::string parse_key(const std::string& text);

