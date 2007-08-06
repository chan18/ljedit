// parse_task.h
// 
#ifndef LJCSPY_INC_PARSE_TASK_H
#define LJCSPY_INC_PARSE_TASK_H

#include "parser.h"
#include <deque>

#ifdef WIN32
#include <windows.h>
void sleep(int sec) { ::Sleep(sec * 1000); }
#else
#include <pthread.h>
#endif

class ParseTask {
public:
	void run() {
#ifdef WIN32
		DWORD tid = 0;
		pid_ = ::CreateThread(NULL, 0, &ParseTask::wrap_thread, this, 0, &tid);
#else
		pthread_create(&pid_, NULL, &ParseTask::wrap_thread, this);
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
		query_queue_.push_back(filename);
	}

	bool has_result() const { return !result_queue_.empty(); }

	cpp::File* query() {
		assert( has_result() );
		cpp::File* result = result_queue_[0];
		result_queue_.pop_front();
		return result;
	}

private:
#ifdef WIN32
	static DWORD WINAPI wrap_thread(LPVOID args) { return ((ParseTask*)args)->thread(); }
#else
	static void* wrap_thread(void* args)		 { return ((ParseTask*)args)->thread(); }
#endif

#ifdef WIN32
	DWORD thread()
#else
	void* thread()
#endif
	{
		stopsign_ = false;

		while( !stopsign_ ) {
			if( query_queue_.empty() ) {
				sleep(1);
				continue;
			}

			std::string filename = query_queue_[0];
			query_queue_.pop_front();

			cpp::File* file = ljcs_parse(filename, &stopsign_);

			result_queue_.push_back(file);
		}

		return 0;
	}

private:
	std::deque<std::string>	query_queue_;
	std::deque<cpp::File*>	result_queue_;

	bool					stopsign_;

#ifdef WIN32
	HANDLE					pid_;
#else
	pthread_t				pid_;
#endif
};

#endif//LJCSPY_INC_PARSE_TASK_H

