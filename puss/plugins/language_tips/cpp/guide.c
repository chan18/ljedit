// guide.c
// 
#include "guide.h"

#include "parser.h"
#include "searcher.h"

struct _CppGuide {
	CppParser		parser;
	CppFileParsed	cb_file_parsed;
	gpointer		cb_file_parsed_tag;

	gboolean		enable_search;
	CppSTree		stree;
	
	GRegex*			re_pkg_config_include;
};

static void guide_on_file_insert(CppFile* file, CppGuide* self) {
	stree_insert( &(self->stree), file );

	if( self->cb_file_parsed )
		(*self->cb_file_parsed)(file, self->cb_file_parsed_tag);
}

static void guide_on_file_remove(CppFile* file, CppGuide* self) {
	stree_remove( &(self->stree), file );
}

CppGuide* cpp_guide_new( gboolean enable_macro_replace
			, gboolean enable_search
			, CppFileParsed* cb_file_parsed
			, gpointer cb_file_parsed_tag )
{
	CppGuide* guide;
	
	guide = g_new0(CppGuide, 1);
	guide->cb_file_parsed = cb_file_parsed;
	guide->cb_file_parsed_tag = cb_file_parsed_tag;
	guide->enable_search = enable_search;

	if( guide->enable_search ) {
		guide->parser.cb_tag = guide;
		guide->parser.cb_file_insert = (FileInsertCallback)guide_on_file_insert;
		guide->parser.cb_file_remove = (FileRemoveCallback)guide_on_file_remove;
		stree_init( &(guide->stree) );
	}

	cpp_parser_init( &(guide->parser), enable_macro_replace );

	guide->re_pkg_config_include = g_regex_new("-I(.*?)\\s", 0, 0, 0);

	return guide;
}

void cpp_guide_free(CppGuide* guide) {
	if( guide ) {
		if( guide->enable_search ) {
			guide->parser.cb_tag = 0;
			guide->parser.cb_file_insert = 0;
			guide->parser.cb_file_remove = 0;

			stree_final( &(guide->stree) );
		}

		cpp_parser_final( &(guide->parser) );

		g_regex_unref(guide->re_pkg_config_include);

		g_free(guide);
	}
}

static GList* guide_append_pkg_config_cflags(CppGuide* guide, GList* include_paths, const gchar* cmd) {
	gchar* p;
	gchar* path;
	gchar* std_output = 0;
	gint exit_status = 0;
	GMatchInfo* match_info = 0;
	GList* pl;

	// !!! glib memory leak on win32
	// 
	if( !g_spawn_command_line_sync(cmd, &std_output, 0, &exit_status, 0) )
		return include_paths;

	if( exit_status!=0 || !std_output )
		return include_paths;

	if( g_regex_match(guide->re_pkg_config_include, std_output, 0, &match_info) ) {
		do {
			p = g_match_info_fetch(match_info, 1);
			if( p ) {
				path = cpp_filename_to_filekey(p, strlen(p));
				g_free(p);

				for( pl=include_paths; path && pl; pl=pl->next ) {
					if( g_str_equal(pl->data, path) ) {
						g_free(path);
						path = 0;
					}
				}

				if( path )
					include_paths = g_list_append(include_paths, path);
			}
		} while( g_match_info_next(match_info, 0) );

		g_match_info_free(match_info);
	}

	g_free(std_output);
	return include_paths;
}

void cpp_guide_include_paths_set(CppGuide* guide, const gchar* paths) {
	gchar* path;
	gchar** p;
	gchar** items;
	GList* include_paths = 0;
	GList* pl;

	if( !paths )
		return;

	items = g_strsplit_set(paths, ";\r\n", 0);
	for( p=items; *p; ++p ) {
		if( *p[0]=='\0' )
			continue;

		if( *p[0]=='$' ) {
			include_paths = guide_append_pkg_config_cflags(guide, include_paths, (*p)+1);

		} else {
			path = cpp_filename_to_filekey(*p, strlen(*p));

			for( pl=include_paths; path && pl; pl=pl->next ) {
				if( g_str_equal(pl->data, path) ) {
					g_free(path);
					path = 0;
				}
			}

			if( path )
				include_paths = g_list_append(include_paths, path);
		}
	}
	g_strfreev(items);

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
	spath_free((GList*)spath);
}

void cpp_guide_search_with_callback( CppGuide* guide
			, gpointer spath
			, CppMatched cb
			, gpointer cb_tag
			, CppFile* file
			, gint line
			, gint limit_num
			, gint limit_time )
{
	searcher_search(&(guide->stree), (GList*)spath, cb, cb_tag, file, line, limit_num, limit_time);
}

#define cpp_elem_ref(elem) cpp_file_ref(elem->file)

static void cpp_elem_unref(CppElem* elem) { cpp_file_unref(elem->file); }

static gint cpp_elem_cmp(CppElem* a, CppElem* b, gint use_unique_id) {
	gint res;

	if( a==b )
		return 0;
	if( a==0 )
		return -1;
	if( b==0 )
		return 1;

	res = g_strcmp0(a->name->buf, b->name->buf);
	if( res==0 && !use_unique_id ) {
		res = (gint)(a->type) - (gint)(b->type);
		if( res==0 )
			res = GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
	}

	return res;
}

typedef struct {
	gint		flag;
	GSequence*	seq;
} SeqMatched;

static gboolean matched_into_sequence(CppElem* elem, SeqMatched* tag) {
	CppElem* p;
	GSequenceIter* ps;
	GSequenceIter* pe;
	gint use_unique_id = 0;

	if( elem->type==CPP_ET_KEYWORD )
		if( !(tag->flag & CPP_GUIDE_SEARCH_FLAG_WITH_KEYWORDS) )
			return FALSE;

	if( !tag->seq )
		tag->seq = g_sequence_new(cpp_elem_unref);

	if( !tag->seq )
		return FALSE;

	if( tag->flag & CPP_GUIDE_SEARCH_FLAG_USE_UNIQUE_ID )
		use_unique_id = 1;

	pe = g_sequence_search(tag->seq, elem, cpp_elem_cmp, use_unique_id);
	if( !g_sequence_iter_is_begin(pe) ) {
		ps = pe;
		do {
			ps=g_sequence_iter_prev(ps);
			p = g_sequence_get(ps);
			if( p==elem )
				return FALSE;

			if( cpp_elem_cmp(elem, p, use_unique_id)!=0 )
				break;
			else if( use_unique_id )
				return FALSE;

		} while( !g_sequence_iter_is_begin(ps) );
	}

	cpp_elem_ref(elem);
	g_sequence_insert_sorted(tag->seq, elem, cpp_elem_cmp, 0);
	return TRUE;
}

GSequence* cpp_guide_search( CppGuide* guide
			, gpointer spath
			, gint flag
			, CppFile* file
			, gint line
			, gint limit_num
			, gint limit_time )
{
	SeqMatched tag = { flag, 0 };
	cpp_guide_search_with_callback(guide, spath, matched_into_sequence, &tag, file, line, limit_num, limit_time);
	return tag.seq;
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
		if( len==0 )
			return res;

		paths = g_new(gchar*, 256);
		paths[0] = g_strdup("_:");
		if( g_ascii_isalpha(filename[0]) )
			paths[0][0] = g_ascii_toupper(filename[0]);
		j = 1;

		for( i=3; i<len && j<255; ++i ) {
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

		paths[j] = 0;
		if( j < 255 ) {
			hfd = FindFirstFileW(wbuf, &wfdd);
			if( hfd != INVALID_HANDLE_VALUE ) {
				paths[j++] = g_utf16_to_utf8((gunichar2*)wfdd.cFileName, -1, 0, 0, 0);
				paths[j] = 0;
				FindClose(hfd);
			}
			res = g_build_filenamev(paths);
		}

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

