// parser.h
// 

#ifndef LJCS_PARSER_H
#define LJCS_PARSER_H

#include "ds.h"

class IParserEnviron {
public:
	virtual bool			pe_is_abspath(std::string& filepath) = 0;
	virtual void			pe_format_filekey(std::string& filename) = 0;
	virtual void			pe_get_path(std::string& path, const std::string& filename) = 0;
	virtual void			pe_build_filekey(std::string& filekey, const std::string& path, const std::string& name) = 0;
	virtual cpp::File*		pe_find_parsed(const std::string& filekey) = 0;
	virtual cpp::File*		pe_check_parsed(const std::string& filekey, time_t& mtime) = 0;
	virtual bool			pe_get_content(const std::string& filekey, std::ostream& out)=0;

	// !!! CRUSH, not use them.
	//virtual std::istream*	pe_istream_create(const std::string& filekey) = 0;
	//virtual void			pe_istream_destroy(std::istream* ins) = 0;

public:
	virtual void			pe_file_incref(cpp::File* file) = 0;
	virtual void			pe_file_decref(cpp::File* file) = 0;

	template<class T>
	inline void file_incref_all_elems(T& v) {
		typename T::iterator it = v.begin();
		typename T::iterator end = v.end();
		for( ; it!=end; ++it )
			pe_file_incref(&((*it)->file));
	}

	template<class T>
	inline void file_decref_all_elems(T& v) {
		typename T::iterator it = v.begin();
		typename T::iterator end = v.end();
		for( ; it!=end; ++it )
			pe_file_decref(&((*it)->file));
	}

public:
	virtual void			pe_on_parsed(cpp::File* file) = 0;
};

class LJCSParser {
protected:
	LJCSParser(IParserEnviron& env)
		: env_(env), stopsign_(false) {}

public:
	virtual ~LJCSParser() {}

	static LJCSParser* create(IParserEnviron& env);

	void set_include_paths(const StrVector& paths)
		{ include_paths_ = paths; }

	cpp::File* parse(const std::string& filekey, std::ostream* os=0) {
		cpp::File* result = ljcs_do_parse(filekey, os);
		if( result!=0 )
			env_.pe_on_parsed(result);
		return result;
	}

	void stopsign_set(bool stop=true) { stopsign_ = stop; }
	bool stopsign_is_set() const      { return stopsign_; }

private:
	virtual cpp::File* ljcs_do_parse(const std::string& filekey, std::ostream* os) = 0;

protected:
	IParserEnviron&		env_;
	bool				stopsign_;

	StrVector			include_paths_;
};

#endif//LJCS_PARSER_H

