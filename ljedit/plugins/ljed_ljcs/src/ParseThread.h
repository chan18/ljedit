// parse_thread.h
// 
#ifndef EEINC_PARSE_THREAD_H
#define EEINC_PARSE_THREAD_H

#include <set>

#include "LJCSEnv.h"

// WIN32 : ftp://sourceware.org/pub/pthreads-win32/prebuilt-dll-2-8-0-release
// 

class ParseThread {
private:
	typedef std::set<std::string>	TStrSet;

public:
	ParseThread() : parser_(0) {}

    void run();

    void stop();

    void add(const std::string& filename);

private:
    static void* wrap_thread(void* args);

    void thread();

private:
	Mutex<TStrSet>			set_;
	LJCSParser*				parser_;

    pthread_t				pid_;
};

#endif//EEINC_PARSE_THREAD_H

