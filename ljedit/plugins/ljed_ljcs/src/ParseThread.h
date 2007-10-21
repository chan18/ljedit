// parse_thread.h
// 
#ifndef EEINC_PARSE_THREAD_H
#define EEINC_PARSE_THREAD_H

#include <set>

#include "LJCSEnv.h"

class ParseThread {
private:
	typedef std::set<std::string>	TStrSet;

public:
	ParseThread() : parser_(0) {}

    void run();

    void stop();

    void add(const std::string& filename);

private:
    void thread();

private:
	Mutex<TStrSet>			set_;
	LJCSParser*				parser_;

	Glib::Thread*			thread_;
};

#endif//EEINC_PARSE_THREAD_H

