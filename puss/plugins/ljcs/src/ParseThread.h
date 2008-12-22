// parse_thread.h
// 
#ifndef PUSS_EXTEND_INC_LJCS_PARSETHREAD_H
#define PUSS_EXTEND_INC_LJCS_PARSETHREAD_H

#include <set>

#include "Environ.h"

class ParseThread {
private:
	typedef std::set<std::string>	TStrSet;

public:
	ParseThread() : parser_(0) {}

    void run(Environ* env);

    void stop();

	void add(const std::string& filename) {
		Locker locker(set_);
		set_->insert(filename);
	}

private:
    void thread();

	static gpointer thread_wrapper(gpointer self) {
		((ParseThread*)self)->thread();
		return 0;
	}

private:
	Mutex<TStrSet>	set_;
	LJCSParser*		parser_;
	Environ*		env_;

	GThread*		thread_;
};

#endif//PUSS_EXTEND_INC_LJCS_PARSETHREAD_H

