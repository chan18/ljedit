// utils.h
// 

#ifndef LJINC_FILE_DIR_UTILS_H
#define LJINC_FILE_DIR_UTILS_H

#include <string>

#ifdef WIN32
	#include <direct.h>
#endif

inline const bool is_split_char(char ch) { return ch=='/' || ch=='\\'; }

inline size_t get_abspath_pos(const std::string& s) {
	assert( !s.empty() );

	if( s[0]=='/' )		// /usr/xx
		return 1;

	if( s.size() > 2 ) {
		if( ::isalpha(s[0]) && s[1]==':' && is_split_char(s[2]) )	// x:\xx
			return 3;

		if( s[0]=='\\' && s[1]=='\\' )	// \\host\e$\xx
			return 2;

		const static std::string FILE_URI = "file://";
		const static size_t FILE_URI_SIZE = FILE_URI.size();

		if( s.size() > FILE_URI_SIZE )
			if( s.compare(0, FILE_URI_SIZE, FILE_URI)==0 )
				return FILE_URI_SIZE;
	}

	return 0;
}

const size_t LJCS_MAX_PATH_SIZE = 8192;

inline void ljcs_filepath_to_abspath(std::string& filepath) {
	if( !filepath.empty() )
	{
		// add current path
		if( get_abspath_pos(filepath)==0 ) {
			char buf[LJCS_MAX_PATH_SIZE];
			buf[0] = '\0';
			::getcwd(buf, LJCS_MAX_PATH_SIZE);
			size_t sz = strnlen(buf, LJCS_MAX_PATH_SIZE);
			if( !is_split_char(filepath[0]) ) {
				buf[sz] = '/';
				sz += 1;
			}
			filepath.insert(0, buf, sz);
		}

#ifdef WIN32
		// replace \ with /
		// 
		// if in windows, change case to lower
		// 
		{
			char* it = &filepath[0];
			char* end = it + filepath.size();
			if( filepath.size() > 2 && filepath[0]=='\\' && filepath[1]=='\\')
				it += 2;
			for( ; it < end; ++it ) {
				if( *it=='\\' )
					*it = '/';
				if( *it > 0 && ::isupper(*it) )
					*it = ::tolower(*it);
			}

		}
#endif

		size_t ps = get_abspath_pos(filepath);
		if( ps==0 )
			return;

		// replace /xx/./yy with /xx/yy
		{
			size_t pe = filepath.npos;
			for(;;) {
				pe = filepath.rfind("./", pe);
				if( pe==filepath.npos || pe < ps )
					break;

				if( pe==ps ) {
					filepath.erase(pe, 2);
					break;
				}

				if( filepath[pe-1]=='/' )
					filepath.erase(pe, 2);
				else
					--pe;

			}
		}

		// replace /xx/../yy with /yy
		{
			for(;;) {
				size_t pe = filepath.find("/../", ps);
				if( pe == filepath.npos )
					break;

				size_t pm = filepath.rfind('/', pe-1);
				if( pm == filepath.npos || pm < ps )
					pm = ps - 1;
				filepath.erase(pm + 1, (pe-pm+3));
			}
		}
	}
}

#endif//LJINC_FILE_DIR_UTILS_H

