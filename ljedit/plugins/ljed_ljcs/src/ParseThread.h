// parse_thread.h
// 
#ifndef EEINC_PARSE_THREAD_H
#define EEINC_PARSE_THREAD_H

#include "ljcs/parser.h"

#include <set>

// WIN32 : ftp://sourceware.org/pub/pthreads-win32/prebuilt-dll-2-8-0-release
// 

#include <pthread.h>

#ifdef WIN32
	#include <windows.h>

	#ifndef sleep
		#define sleep(sec)	::Sleep(sec*1000);
	#endif
#endif

class ParseThread {
public:
    void run() {
        pthread_create(&pid_, NULL, &ParseThread::wrap_thread, this);
    }

    void stop() {
        stopsign_ = true;

        pthread_join(pid_, 0);
    }

    void add(const std::string& filename) {
        set_.insert(filename);
    }

private:
    static void* wrap_thread(void* args) {
		((ParseThread*)args)->thread();
		pthread_exit(0);
		return 0;
	}

    void thread()
    {
        stopsign_ = false;

        while( !stopsign_ ) {
            if( set_.empty() ) {
                sleep(1);
                continue;
            }

            std::set<std::string>::iterator it = set_.begin();
            std::string filename = *it;
            set_.erase(it);

            ljcs_parse(filename, &stopsign_);
        }
    }

private:
    std::set<std::string>	set_;

    bool					stopsign_;

    pthread_t				pid_;
};

#endif//EEINC_PARSE_THREAD_H

