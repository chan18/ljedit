// LJCSEnv.h
// 

#ifndef LJED_INC_LJCSENV_H
#define LJED_INC_LJCSENV_H

#include "ljcs/ljcs.h"
#include <pthread.h>
#include <cassert>

class LJEditor;


template<class T>
class RWLock {
public:
	RWLock()					{ pthread_rwlock_init(&lock_, 0); }
	~RWLock()					{ pthread_rwlock_destroy(&lock_); }

	bool tryrdlock() 			{ return pthread_rwlock_tryrdlock(&lock_)==0; }
	bool trywrlock() 			{ return pthread_rwlock_trywrlock(&lock_)==0; }
	void rdlock()    			{ pthread_rwlock_rdlock(&lock_); }
	void wrlock()				{ pthread_rwlock_wrlock(&lock_); }
	bool timedrdlock(long ms)	{ timespec ts = { ms/1000, ms%1000 }; return pthread_rwlock_timedrdlock(&lock_, &ts)==0; }
	bool timedwrlock(long ms)	{ timespec ts = { ms/1000, ms%1000 }; return pthread_rwlock_timedwrlock(&lock_, &ts)==0; }
	void unlock()				{ pthread_rwlock_unlock(&lock_); }

	T    get()					{ rdlock(); T r=value_; unlock(); return r; }
	void set(const T& o)		{ wrlock(); value_ = o; unlock(); }

	T&   value()				{ return value_; }
	T*   operator->()           { return &value_; }

private:
	T							value_;
	pthread_rwlock_t			lock_;
};

template<class T>
struct RdLocker {
	RdLocker(RWLock<T>& lock) : lock_(lock)	{ lock_.rdlock(); }
	~RdLocker()								{ lock_.unlock(); }

	RWLock<T>& lock_;
};

template<class T>
struct WrLocker {
	WrLocker(RWLock<T>& lock) : lock_(lock)	{ lock_.wrlock(); }
	~WrLocker()								{ lock_.unlock(); }

	RWLock<T>& lock_;
};

template<class T>
class Mutex {
public:
	Mutex()						{ pthread_mutex_init(&mutex_, 0); }
	~Mutex()					{ pthread_mutex_destroy(&mutex_); }

	bool trylock() 				{ return ::pthread_mutex_trylock(&mutex_)==0; }
	void lock()    				{ pthread_mutex_lock(&mutex_); }
	bool timedlock(size_t ms)	{ timespec ts = { ms/1000, ms%1000 }; return pthread_mutex_timedlock(&mutex_, &ts)==0; }
	void unlock()				{ pthread_mutex_unlock(&mutex_); }

	T    get()					{ lock(); T r=value_; unlock(); return r; }
	void set(const T& o)		{ lock(); value_ = o; unlock(); }

	T&   value()				{ return value_; }
	T*   operator->()           { return &value_; }

private:
	T							value_;
	pthread_mutex_t				mutex_;
};

template<class T>
struct Locker {
	Locker(Mutex<T>& mutex) : mutex_(mutex)	{ mutex_.lock(); }
	~Locker()								{ mutex_.unlock(); }

	Mutex<T>& mutex_;
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
	StrVector				get_include_paths() { return include_paths_.get(); }
	StrVector				get_pre_parse_files() { return pre_parse_files_.get(); }

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

