// LJCSEnv.h
// 

#ifndef LJED_INC_LJCSENV_H
#define LJED_INC_LJCSENV_H

#include "ljcs/ljcs.h"
#include <gtkmm.h>
#include <cassert>

class LJEditor;


template<class T>
class RWLock : public Glib::RWLock {
public:
	T    copy()					{ Glib::RWLock::ReaderLock locker(*this); return value_; }
	void set(const T& o)		{ Glib::RWLock::WriterLock locker(*this); value_ = o; }

	T&   ref()					{ return value_; }
	T*   operator->()           { return &value_; }

private:
	T							value_;
};

template<class T>
class Mutex : public Glib::Mutex {
public:
	T    copy()					{ Glib::Mutex::Lock locker(*this); return value_; }
	void set(const T& o)		{ Glib::Mutex::Lock locker(*this); value_ = o; }

	T&   ref()					{ return value_; }
	T*   operator->()           { return &value_; }

private:
	T			value_;
};

class LJCSEnv : public IParserEnviron {
public:
	static LJCSEnv& self();
	
	void init(LJEditor& ljedit) { ljedit_ = &ljedit; }

	LJEditor& ljedit()		{ assert(ljedit_!=0); return *ljedit_; }

private:
	LJCSEnv();
	~LJCSEnv();

	LJCSEnv(const LJCSEnv& o);				// no implement
	LJCSEnv& operator = (const LJCSEnv& o);	// no implement

public:
	StrVector				get_include_paths() { return include_paths_.copy(); }
	StrVector				get_pre_parse_files() { return pre_parse_files_.copy(); }

	void					set_include_paths(const StrVector& paths);
	void					set_pre_parse_files(const StrVector& files);

	cpp::File*				find_parsed(const std::string& filekey);					// get and ref
	cpp::File*				find_parsed_in_include_path(const std::string& filename);	// get and ref

	RWLock<cpp::STree>&		stree() { return index_; }

public:
	bool					in_include_path(const std::string& path);

	void					re_make_index();

private:
	void					add_parsed(cpp::File* file);			// add and ref
	void					remove_parsed(cpp::File* file);			// remove and unref
	void					remove_parsed(const std::string& file);	// remove and unref

private:	// IParserEnviron
	virtual bool			pe_is_abspath(std::string& filepath);
	virtual void			pe_format_filekey(std::string& filename);
	virtual void			pe_get_path(std::string& path, const std::string& filename);
	virtual void			pe_build_filekey(std::string& filekey, const std::string& path, const std::string& name);
	virtual cpp::File*		pe_find_parsed(const std::string& filekey);
	virtual cpp::File*		pe_check_parsed(const std::string& filekey, time_t& mtime);
	virtual bool			pe_get_content(const std::string& filekey, std::ostream& out);

	virtual void			pe_on_parsed(cpp::File* file);

private:
	LJEditor*				ljedit_;

	RWLock<StrVector>		include_paths_;

	RWLock<StrVector>		pre_parse_files_;

	RWLock<cpp::FileMap>	parsed_files_;

	RWLock<cpp::STree>		index_;
};

#endif//LJED_INC_LJCSENV_H

