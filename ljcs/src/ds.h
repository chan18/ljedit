// ds.h
// 

#ifndef LJCS_DS_H
#define LJCS_DS_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <cassert>

typedef std::vector<std::string>	StrVector;

namespace cpp {

const char NORMAL_VIEW    = 'n';
const char PUBLIC_VIEW    = 'p';
const char PROTECTED_VIEW = 'o';
const char PRIVATE_VIEW   = 'r';

const char ET_KEYWORD     = 'K';
const char ET_UNDEF       = 'U';
const char ET_MACRO       = 'D';
const char ET_INCLUDE     = 'I';
const char ET_VAR         = 'v';
const char ET_FUN         = 'f';
const char ET_ENUMITEM    = 'i';
const char ET_ENUM        = 'e';
const char ET_CLASS       = 'c';
const char ET_USING       = 'u';
const char ET_NAMESPACE   = 'n';
const char ET_TYPEDEF     = 't';
const char ET_NCSCOPE     = 'S';	// namespace or class

class Element;
class Scope;
class File;

typedef std::vector<Element*>					Elements;
typedef std::set<cpp::Element*>					ElementSet;
typedef std::vector<File*>						Files;
typedef std::set<cpp::File*>					FileSet;
typedef std::map<std::string, File*>			FileMap;

class Element {
protected:
	Element(File& f, char t, const std::string& n, size_t s, size_t e, char v)
		: file(f)
		, type(t)
		, view(v)
		, name(n)
		, sline(s)
		, eline(e)
	{}

public:
	template<typename T>
	static T* create(File& file, const std::string& name, size_t sline, size_t eline, char view=NORMAL_VIEW) {
		T* p = new T(file, name, sline, eline, view);
		if( p==0 )
			throw std::runtime_error("no enough memeory");
		return p;
	}

	virtual ~Element() {}

public:
	File&		file;

	char		type;
	char		view;
	std::string	name;
	size_t		sline;
	size_t		eline;
	std::string	decl;
};

class Include;
class Using;

typedef std::vector<Include*>					Includes;
typedef std::vector<Using*>						Usings;

inline void clear_elements(Elements& elems) {
	Elements::iterator it = elems.begin();
	Elements::iterator end = elems.end();
	for( ; it!=end; ++it )
		delete (*it);
	elems.clear();
}

class Scope {
public:
	Scope() {}

	~Scope() {
		clear_elements(elems);
	}

	bool empty() const { return elems.empty(); }

public:
	Elements    elems;	// need delete

public:	// for index
	//Usings		usings;	// not use delete
};

class File {
public:
	File(const std::string& fname) : filename(fname), ref_count(0) {}
	~File() {}

private:
	// nocopyable
	File(const File& o);
	File& operator = (const File& o);

public:
	std::string		filename;
	time_t			datetime;

	Scope			scope;

public:
	volatile int	ref_count;
};

class KeywordElement : public Element {
public:
	KeywordElement(File& file, const std::string& name)
		: Element(file, ET_KEYWORD, name, 0, 0, cpp::NORMAL_VIEW) {}
};

class Macro : public Element {
public:
	typedef StrVector TArgs;

public:
	Macro(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_MACRO, name, sline, eline, view)
		, args(0) {}

	~Macro() {
		if( !empty_args() )
			delete args;
		args = 0;
	}

	bool empty_args() const { return args==ref_empty_args(); }

	void set_empty_args()	{
		assert( args==0 );
		args = ref_empty_args();
	}

private:
	TArgs* ref_empty_args() const {
		static TArgs empty_args_;
		return &empty_args_;
	}

public:
	TArgs*			args;
	std::string		value;
};

class Undef : public Element {
public:
	Undef(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_UNDEF, name, sline, eline, view) {}
};

class Include : public Element {
public:
	Include(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_INCLUDE, "include", sline, eline, view) {}

public:
	std::string	filename;
	bool		sys_header;
	std::string	include_file;
};

class Var : public Element {
public:
	Var(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_VAR, name, sline, eline, view) {}

public:
	std::string	typekey;
	std::string	nskey;
};

class Template {
public:
	struct Arg {
		std::string type;
		std::string name;
		std::string value;
	};

	typedef std::vector<Arg> TArgs;

public:
	TArgs args;
};

class Function : public Element {
public:
	Function(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_FUN, name, sline, eline, view) {}

public:
	std::string	nskey;
	std::string	typekey;
	bool		fun_ptr;
	Template*	templ;
	Scope		impl;
};

class NCScope : public Element {
public:
	NCScope(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_NCSCOPE, name, sline, eline, view) {}

public:
	Scope		scope;
};

class Namespace : public NCScope {
public:
	Namespace(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: NCScope(file, name, sline, eline, view) { type = ET_NAMESPACE; }
};

class Class : public NCScope {
public:
	Class(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: NCScope(file, name, sline, eline, view) { type = ET_CLASS; }

public:
	std::string	nskey;
	StrVector	inhers;
};

class EnumItem : public Element {
public:
	EnumItem(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_ENUMITEM, name, sline, eline, view) {}

public:
	std::string value;
};

class Enum : public NCScope {
public:
	Enum(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: NCScope(file, name, sline, eline, view) { type = ET_ENUM; }

public:
	std::string	nskey;
};

class Using : public Element {
public:
	Using(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_USING, name, sline, eline, view) {}

public:
	bool		isns;
	std::string	nskey;
};

class Typedef : public Element {
public:
	Typedef(File& file, const std::string& name, size_t sline, size_t eline, char view)
		: Element(file, ET_TYPEDEF, name, sline, eline, view) {}

public:
	std::string	typekey;
};

}// namespace cpp

#endif//LJCS_DS_H

