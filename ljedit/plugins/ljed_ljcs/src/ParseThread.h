// parse_thread.h
// 
#ifndef EEINC_PARSE_THREAD_H
#define EEINC_PARSE_THREAD_H

#include "ljcs/parser.h"

#include <set>

#ifdef WIN32
    #include <windows.h>
    inline void sleep(int sec) { ::Sleep(sec * 1000); }
#else
    #include <pthread.h>
#endif

class ParseThread {
public:
    void run() {
#ifdef WIN32
        DWORD tid = 0;
        pid_ = ::CreateThread(NULL, 0, &ParseThread::wrap_thread, this, 0, &tid);
#else
        pthread_create(&pid_, NULL, &ParseThread::wrap_thread, this);
#endif
    }

    void stop() {
        stopsign_ = true;
#ifdef WIN32
        ::CloseHandle(pid_);
        ::WaitForSingleObject(pid_, 5000);
#else
        pthread_join(pid_, 0);
#endif
    }

    void add(const std::string& filename) {
        set_.insert(filename);
    }

private:
#ifdef WIN32
    static DWORD WINAPI wrap_thread(LPVOID args) { return ((ParseThread*)args)->thread(); }
#else
    static void* wrap_thread(void* args)		 { return ((ParseThread*)args)->thread(); }
#endif

#ifdef WIN32
    DWORD thread()
#else
    void* thread()
#endif
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

        return 0;
    }

private:
    std::set<std::string>	set_;

    bool					stopsign_;

#ifdef WIN32
    HANDLE					pid_;
#else
    pthread_t				pid_;
#endif
};

#endif//EEINC_PARSE_THREAD_H

