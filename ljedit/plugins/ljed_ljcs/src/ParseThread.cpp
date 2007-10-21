// parse_thread.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "ParseThread.h"

#ifdef WIN32
	#include <windows.h>

	#ifndef sleep
		#define sleep(sec)	::Sleep(sec*1000);
	#endif
#endif


void ParseThread::run() {
	parser_ = LJCSParser::create(LJCSEnv::self());
	if( parser_ == 0 )
		return;

	thread_ = Glib::Thread::create( sigc::mem_fun(this, &ParseThread::thread), true );
}

void ParseThread::stop() {
	parser_->stopsign_set();

	thread_->join();

	delete parser_;
	parser_ = 0;
}

void ParseThread::add(const std::string& filename) {
	Glib::Mutex::Lock locker(set_);
    set_->insert(filename);
}

void ParseThread::thread()
{
	LJCSEnv& env = LJCSEnv::self();
    parser_->stopsign_set(false);

	std::string filekey;

	// include paths
	{
		parser_->set_include_paths(env.get_include_paths());
	}

	// parse pre parse files
	{
		StrVector vec = env.get_pre_parse_files();

		StrVector::reverse_iterator it = vec.rbegin();
		StrVector::reverse_iterator end = vec.rend();
		for( ; it!=end; ++it ) {
			cpp::File* file = parser_->parse(*it);
			if( file != 0 )
				file->unref();
		}
	}

    while( !parser_->stopsign_is_set() ) {
        if( set_->empty() ) {
            sleep(1);
            continue;
        }

		// pop first filename
		{
			Glib::Mutex::Lock locker(set_);
            TStrSet::iterator it = set_->begin();
			if( it==set_->end() )
				continue;

	        filekey = *it;
			set_->erase(it);
		}

		// TODO : move make_index into pe_on_parsed()
		// 

		// parse and make index
		{
			Glib::RWLock::WriterLock index_locker(env.stree());

			cpp::File* file = parser_->parse(filekey);
			if( file != 0 )
				file->unref();

			env.re_make_index();
		}
    }
}

