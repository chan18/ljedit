// main.cpp
//

#include "StdAfx.h"	// for vc precompile header

#include "LJEditorImpl.h"
#include "PluginManager.h"

#ifdef G_OS_WIN32
	#include <windows.h>
#endif

int already_running();

int main(int argc, char *argv[]) {
	const char* open_filename = 0;
	if( argc > 1 )
		open_filename = argv[1];

	// singleton pid
	if( already_running() ) {
		// TODO : notify ljedit open file
		
	} else {
		Gtk::Main kit(argc, argv);
		gtksourceview::init();

		std::string path;
		
	#ifdef G_OS_WIN32
		path.resize(4096);
		path.resize( GetModuleFileNameA(0, &path[0], 4096) );
		path.erase( path.rfind("\\") );
		path = Glib::locale_to_utf8(path);

	#else
		path = argv[0];
		path.erase(path.find_last_of('/'));

	#endif

		PluginManager::self().add_plugins_path(path + "/plugins");

		LJEditorImpl& app = LJEditorImpl::self();
		if( app.create(path) ) {
			app.add_open_file(open_filename);
			app.run();
			app.destroy();
		}
	}

    return 0;
}


#ifdef WIN32
	#include <windows.h>
	#include <tchar.h>

	int already_running() {
		HANDLE m_hMutex = CreateMutex(NULL, FALSE, _T("ljedit_lock"));
		if( GetLastError() == ERROR_ALREADY_EXISTS )
			return 1;

		return 0;
	}

#else
	#include <unistd.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <syslog.h>
	#include <string.h>
	#include <errno.h>
	#include <stdio.h>
	#include <sys/stat.h>

	int lock_reg(int fd, off_t offset, int whence, off_t len) {
		struct flock lock;

		lock.l_type = F_WRLCK;
		lock.l_start = offset;
		lock.l_whence = whence;
		lock.l_len = len;

		return (fcntl(fd, F_SETLK, &lock));
	}

	int already_running() {
		int fd, val;
		char buf[16];

		char pid_file[256] = { '\0' };
		strncpy(pid_file, "/tmp/ljedit.pid.", 256);

		sprintf(buf, "%d", getuid());
		strncat(pid_file, buf, 256);

		// TODO : same user multi-login
		// ...

		if( (fd = open(pid_file, O_WRONLY | O_CREAT, 0644)) < 0 ) {
			perror("error[open lock file]");
			exit(1);
		}

		// try and set a write lock on the entire file
		if( lock_reg(fd, 0, SEEK_SET, 0) < 0 ) {
			if(errno == EACCES || errno == EAGAIN) {
				return 1;

			}else {
				perror("error[write_lock]");
				close(fd);
				exit(1);
			}
		}

		// truncate to zero length, now that we have the lock
		if( ftruncate(fd, 0) < 0 ) {
			perror("error[ftruncate]");
			close(fd);      
			exit(1);
		}

		// write our process id
		sprintf(buf, "%d\n", getpid());
		if( write(fd, buf, strlen(buf)) != strlen(buf) ) {
			perror("error[write]");
			close(fd);      
			exit(1);
		}

		// set close-on-exec flag for descriptor
		if((val = fcntl(fd, F_GETFD, 0) < 0 )) {
			perror("error[fcntl]");
			close(fd);
			exit(1);
		}

		val |= FD_CLOEXEC;
		if( fcntl(fd, F_SETFD, val) < 0 ) {
			perror("error[fcntl again]");
			close(fd);
			exit(1);
		}

		// leave file open until we terminate: lock will be held
		return 0;
	}

#endif


/*
#include <pcre++.h>

void test_pcrepp() {
    try {
        // PCRE

        // \w	Match a "word" character (alphanumeric plus "_")
        // \W	Match a non-"word" character
        // \s	Match a whitespace character
        // \S	Match a non-whitespace character
        // \d	Match a digit character
        // \D	Match a non-digit character
        // \pP	Match P, named property.  Use \p{Prop} for longer names.
        // \PP	Match non-P
        // \X	Match eXtended Unicode "combining character sequence",
        //     equivalent to (?:\PM\pM*)
        // \C	Match a single C char (octet) even under Unicode.
        // NOTE: breaks up characters into their UTF-8 bytes,
        // so you may end up with malformed pieces of UTF-8.
        // Unsupported in lookbehind.

        // POSIX

        // alpha       IsAlpha
        // alnum       IsAlnum
        // ascii       IsASCII
        // blank       IsSpace
        // cntrl       IsCntrl
        // digit       IsDigit        \d
        // graph       IsGraph
        // lower       IsLower
        // print       IsPrint
        // punct       IsPunct
        // space       IsSpace
        //             IsSpacePerl    \s
        // upper       IsUpper
        // word        IsWord
        // xdigit      IsXDigit

        std::string exp = "([[:alpha:]]*) (\\d*)";
        pcrepp::Pcre re(exp);
        if( re.search("abc 123") ) {
            std::string s0 = re[0];
            std::string s1 = re[1];
            std::string s2 = re[2];
            int a = 0;
        }
    } catch(pcrepp::Pcre::exception e) {
        int b = 0;
    }
}
*/

