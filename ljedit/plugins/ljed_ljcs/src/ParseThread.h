// parse_thread.h
// 
#ifndef EEINC_PARSE_THREAD_H
#define EEINC_PARSE_THREAD_H

#include <set>

#include "LJCSEnv.h"

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

			cpp::File* old_file = ParserEnviron::self().find_parsed(filename);
			if( old_file!=0 )
				old_file->ref();

			cpp::File* new_file = ljcs_parse(filename, &stopsign_);

			::pthread_rwlock_wrlock(&LJCSEnv::self().stree_rwlock);
			//printf("sssssssssssssssssssssssssssssss\n");

			if( old_file!=0 ) {
				if( old_file != new_file ) {
					LJCSEnv::self().stree.remove(*old_file);
				}
				old_file->unref();
			}

			if( new_file!=0 ) {
				LJCSEnv::self().stree.add(*new_file);
			}

			::pthread_rwlock_unlock(&LJCSEnv::self().stree_rwlock);
			//printf("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
        }
    }

private:
    std::set<std::string>	set_;

    bool					stopsign_;

    pthread_t				pid_;
};

#endif//EEINC_PARSE_THREAD_H

