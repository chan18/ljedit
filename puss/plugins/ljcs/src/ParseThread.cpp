// parse_thread.cpp
// 

#include "ParseThread.h"


void ParseThread::run(Environ* env) {
	env_ = env;

	parser_ = LJCSParser::create(*env);
	if( parser_ == 0 )
		return;

	thread_ = g_thread_create( &ParseThread::thread_wrapper, this, TRUE, 0);
}

void ParseThread::stop() {
	parser_->stopsign_set();

	g_thread_join(thread_);

	delete parser_;
	parser_ = 0;
}

void ParseThread::thread() {
    parser_->stopsign_set(false);

	std::string filekey;

	// include paths
	{
		parser_->set_include_paths(env_->get_include_paths());
	}

	// parse pre parse files
	{
		StrVector vec = env_->get_pre_parse_files();

		StrVector::reverse_iterator it = vec.rbegin();
		StrVector::reverse_iterator end = vec.rend();
		for( ; it!=end; ++it ) {
			cpp::File* file = parser_->parse(*it);
			if( file != 0 )
				env_->file_decref(file);
		}
	}

    while( !parser_->stopsign_is_set() ) {
        if( set_->empty() ) {
			g_usleep(1000);
            continue;
        }

		// pop first filename
		{
			Locker locker(set_);
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
			WLocker locker(env_->stree());

			cpp::File* file = parser_->parse(filekey);
			if( file != 0 )
				env_->file_decref(file);

			env_->re_make_index();
		}
    }
}

