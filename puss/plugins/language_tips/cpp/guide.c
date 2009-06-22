// guide.c
// 
#include "guide.h"

#include "parser.h"
#include "searcher.h"

struct _CppGuide {
	CppParser	parser;
	CppSTree	stree;
};

static void guide_on_file_insert(CppFile* file, CppGuide* self) {
	stree_insert( &(self->stree), file );
}

static void guide_on_file_remove(CppFile* file, CppGuide* self) {
	stree_remove( &(self->stree), file );
}

CppGuide* cpp_guide_new(gboolean enable_macro_replace, gboolean enable_search) {
	CppGuide* guide = g_new0(CppGuide, 1);

	guide->parser.cb_tag = guide;
	guide->parser.cb_file_insert = (FileInsertCallback)guide_on_file_insert;
	guide->parser.cb_file_remove = (FileRemoveCallback)guide_on_file_remove;

	cpp_parser_init( &(guide->parser), enable_macro_replace );

	stree_init( &(guide->stree) );

	return guide;
}

void cpp_guide_free(CppGuide* guide) {
	if( guide ) {
		cpp_parser_final( &(guide->parser) );

		stree_final( &(guide->stree) );
	}
}

void cpp_guide_include_paths_set(CppGuide* guide, const gchar* paths) {
	gchar** p;
	gchar** items;
	GList* include_paths = 0;

	if( paths ) {
		items = g_strsplit_set(paths, ";\r\n", 0);
		for( p=items; *p; ++p ) {
			if( *p[0]=='\0' )
				continue;
			include_paths = g_list_append(include_paths, *p);
		}
		g_strfreev(items);
	}

	cpp_parser_include_paths_set(&(guide->parser), include_paths);
}

CppIncludePaths* cpp_guide_include_paths_ref(CppGuide* guide) {
	return cpp_parser_include_paths_ref( &(guide->parser) );
}

void cpp_guide_include_paths_unref(CppIncludePaths* paths) {
	cpp_parser_include_paths_unref(paths);
}

CppFile* cpp_guide_find_parsed(CppGuide* guide, const gchar* filename, gint namelen) {
	gchar* filekey = cpp_filename_to_filekey(filename, namelen);
	CppFile* file = cpp_parser_find_parsed(&(guide->parser), filekey);
	g_free(filekey);
	return file;
}

CppFile* cpp_guide_parse(CppGuide* guide, const gchar* filename, gint namelen, gboolean force_rebuild) {
	gchar* filekey = cpp_filename_to_filekey(filename, namelen);
	CppFile* file = cpp_parser_parse(&(guide->parser), filekey, force_rebuild);
	g_free(filekey);
	return file;
}

gpointer cpp_spath_find( gboolean find_startswith
			, gchar (*do_prev)(gpointer it)
			, gchar (*do_next)(gpointer it)
			, gpointer start_iter
			, gpointer end_iter )
{
	SearchIterEnv env = { do_prev, do_next };
	return spath_find(&env, start_iter, end_iter, find_startswith);
}

gpointer cpp_spath_parse(gboolean find_startswith, const gchar* text) {
	return spath_parse(text, find_startswith);
}

void cpp_spath_free(gpointer spath) {
	return spath_free((GList*)spath);
}

void cpp_guide_search(CppGuide* guide
			, gpointer spath
			, CppMatched cb
			, gpointer cb_tag
			, CppFile* file
			, gint line )
{
	searcher_search(&(guide->stree), (GList*)spath, cb, cb_tag, file, line);
}


#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

gchar* cpp_filename_to_filekey(const gchar* filename, glong namelen) {
	gchar* res = 0;

#ifdef G_OS_WIN32
	WIN32_FIND_DATAW wfdd;
	HANDLE hfd;
	wchar_t wbuf[32768];
	gchar**  paths;
	wchar_t* wfname;
	gsize len;
	gsize i;
	gsize j;
	
	wfname = (wchar_t*)g_utf8_to_utf16(filename, namelen, 0, 0, 0);
	if( wfname ) {
		len = GetFullPathNameW(wfname, 32768, wbuf, 0);
		len = GetLongPathNameW(wbuf, wbuf, 32768);
		g_free(wfname);

		paths = g_new(gchar*, 256);
		paths[0] = g_strdup("_:");
		paths[0][0] = g_ascii_toupper(filename[0]);
		j = 1;

		for( i=3; i<len; ++i ) {
			if( wbuf[i]=='\\' ) {
				wbuf[i] = '\0';

				hfd = FindFirstFileW(wbuf, &wfdd);
				if( hfd != INVALID_HANDLE_VALUE ) {
					paths[j++] = g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
					FindClose(hfd);
				}
				wbuf[i] = '\\';
			}
		}

		hfd = FindFirstFileW(wbuf, &wfdd);
		if( hfd != INVALID_HANDLE_VALUE ) {
			paths[j++] = g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
			FindClose(hfd);
		}
		paths[j] = 0;

		res = g_build_filenamev(paths);
		g_strfreev(paths);
	}

	if( !res )
		res = g_strdup(filename);

	g_assert( res );

#else
	gboolean succeed = TRUE;
	gchar** p;
	gchar* path;
	gchar* outs[256];
	gchar** pt = outs;
	gchar** paths = g_strsplit(filename, "/", 0);
	for( p=paths; succeed && *p; ++p ) {
		path = *p;
		if( g_str_equal(*p, ".") ) {
			// ignore ./

		} else if( g_str_equal(*p, "..") ) {
			if( pt==outs )
				succeed = FALSE;
			else
				--pt;

		} else {
			*pt = *p;
			++pt;
		}
	}

	if( succeed ) {
		*pt = NULL;
		res = g_strjoinv("/", outs);

	} else {
		res = g_strdup(filename);
	}

	g_strfreev(paths);

#endif

	return res;
}

