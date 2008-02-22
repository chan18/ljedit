// Environ.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_ENVIRON_H
#define PUSS_EXTEND_INC_LJCS_ENVIRON_H

#include "IPuss.h"
#include "ljcs/ljcs.h"

#include <glib/gthread.h>

struct _RWLock {
	_RWLock()	{ g_static_rw_lock_init(&lock); }
	~_RWLock()	{ g_static_rw_lock_free(&lock); }
	GStaticRWLock	lock;
};

struct RLocker {
	RLocker(_RWLock& lock) : _ref(lock)
		{ g_static_rw_lock_reader_lock(&_ref.lock); }

	~RLocker()
		{ g_static_rw_lock_reader_unlock(&_ref.lock); }

	_RWLock&	_ref;
};

struct WLocker {
	WLocker(_RWLock& lock) : _ref(lock)
		{ g_static_rw_lock_writer_lock(&_ref.lock); }

	~WLocker()
		{ g_static_rw_lock_writer_unlock(&_ref.lock); }

	_RWLock&	_ref;
};

template<class TValue>
class RWLock : public _RWLock {
public:
	TValue copy() { RLocker locker(*this);	return value_; }

	void set(const TValue& o) { WLocker locker(*this); value_ = o; }

	TValue& ref() { return value_; }

	TValue* operator->() { return &value_; }

private:
	TValue			value_;
};

struct _Mutex {
	_Mutex()	{ g_static_mutex_init(&lock); }
	~_Mutex()	{ g_static_mutex_free(&lock); }
	GStaticMutex	lock;
};

struct Locker {
	Locker(_Mutex& lock) : _ref(lock)
		{ g_static_mutex_lock(&_ref.lock); }

	~Locker()
		{ g_static_mutex_unlock(&_ref.lock); }

	_Mutex& _ref;
};

template<class TValue>
class Mutex : public _Mutex{
public:
	TValue copy() { Locker locker(*this); return value_; }

	void set(const TValue& o) { Locker locker(*this); value_ = o; }

	TValue& ref() { return value_; }

	TValue* operator->() { return &value_; }

private:
	TValue			value_;
};

class Environ : public IParserEnviron {
public:
	Environ();
	~Environ();

public:
	StrVector	get_include_paths()   { return include_paths_.copy(); }
	StrVector	get_pre_parse_files() { return pre_parse_files_.copy(); }

	void		set_include_paths(const StrVector& paths);
	void		set_pre_parse_files(const StrVector& files);

	cpp::File*	find_parsed(const std::string& filekey);					// get and ref
	cpp::File*	find_parsed_in_include_path(const std::string& filename);	// get and ref

	void		file_incref(cpp::File* file);
	void		file_decref(cpp::File* file);

	RWLock<cpp::STree>&	stree() { return index_; }

public:
	bool	in_include_path(const std::string& path);

	void	re_make_index();

	void	find_keyword(const std::string& key, IMatched& cb);

private:
	void	add_parsed(cpp::File* file);			// add and ref
	void	remove_parsed(cpp::File* file);			// remove and unref
	void	remove_parsed(const std::string& file);	// remove and unref

private:	// IParserEnviron
	virtual bool		pe_is_abspath(std::string& filepath);
	virtual void		pe_format_filekey(std::string& filename);
	virtual void		pe_get_path(std::string& path, const std::string& filename);
	virtual void		pe_build_filekey(std::string& filekey, const std::string& path, const std::string& name);
	virtual cpp::File*	pe_find_parsed(const std::string& filekey);
	virtual cpp::File*	pe_check_parsed(const std::string& filekey, time_t& mtime);
	virtual bool		pe_get_content(const std::string& filekey, std::ostream& out);

	virtual void		pe_file_incref(cpp::File* file) { file_incref(file); }
	virtual void		pe_file_decref(cpp::File* file) { file_decref(file); }

	virtual void		pe_on_parsed(cpp::File* file);

private:
	Puss*					app_;

	RWLock<StrVector>		include_paths_;

	RWLock<StrVector>		pre_parse_files_;

	RWLock<cpp::FileMap>	parsed_files_;

	RWLock<cpp::STree>		index_;

	cpp::File				keywords_;
};

#endif//PUSS_EXTEND_INC_LJCS_ENVIRON_H

