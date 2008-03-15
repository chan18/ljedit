// Utils.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_UTILS_H
#define PUSS_EXTEND_INC_LJCS_UTILS_H

#include <glib/gthread.h>

struct _RWLock {
	_RWLock()	{ g_static_rw_lock_init(&lock); }
	~_RWLock()	{ g_static_rw_lock_free(&lock); }
	
	gboolean reader_try_lock() { return g_static_rw_lock_reader_trylock(&lock); }
	void reader_lock() { g_static_rw_lock_reader_lock(&lock); }
	void reader_unlock() { g_static_rw_lock_reader_unlock(&lock); }

	void writer_lock() { g_static_rw_lock_writer_lock(&lock); }
	void writer_unlock() { g_static_rw_lock_writer_unlock(&lock); }

	GStaticRWLock	lock;
};

struct RLocker {
	RLocker(_RWLock& lock) : _ref(lock) { _ref.reader_lock(); }

	~RLocker() { _ref.reader_unlock(); }

	_RWLock&	_ref;
};

struct WLocker {
	WLocker(_RWLock& lock) : _ref(lock) { _ref.writer_lock(); }

	~WLocker() { _ref.writer_unlock(); }

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
class Mutex : public _Mutex {
public:
	TValue copy() { Locker locker(*this); return value_; }

	void set(const TValue& o) { Locker locker(*this); value_ = o; }

	TValue& ref() { return value_; }

	TValue* operator->() { return &value_; }

private:
	TValue			value_;
};

class Cond {
public:
	GCond* gcond;

	Cond()  { gcond = g_cond_new(); }
	~Cond() { g_cond_free(gcond); }

	void signal() { g_cond_signal(gcond); }
	void wait(_Mutex& mutex)   { g_cond_wait(gcond, g_static_mutex_get_mutex(&mutex.lock)); }

};

#include <gtk/gtk.h>
#include "ljcs/ljcs.h"

class Environ;

class DocIter_Gtk : public IDocIter {
public:
    DocIter_Gtk(GtkTextIter* it)
        : IDocIter( (char)gtk_text_iter_get_char(it) )
        , it_(it) {}

protected:
    virtual char do_prev() {
		return gtk_text_iter_backward_char(it_)
			? (char)gtk_text_iter_get_char(it_)
			: '\0';
    }

    virtual char do_next() {
		return gtk_text_iter_forward_char(it_)
			? (char)gtk_text_iter_get_char(it_)
			: '\0';
    }

private:
    GtkTextIter*	it_;
};

gboolean is_in_comment(GtkTextIter* it);

gboolean find_keys( StrVector& keys
    , GtkTextIter* it
    , GtkTextIter* end
    , cpp::File* file
	, gboolean find_startswith);

size_t find_best_matched_index(cpp::Elements& elems);

cpp::Element* find_best_matched_element(cpp::ElementSet& eset);

GtkTextBuffer* set_cpp_lang_to_source_view(GtkTextView* source_view);

#endif//PUSS_EXTEND_INC_LJCS_UTILS_H

