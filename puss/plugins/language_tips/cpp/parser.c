// parser.c
// 

#include "parser.h"
#include <sys/stat.h>

#include "keywords.h"
#include "cps.h"


static void __dump_block(Block* block) {
	gchar ch;
	gsize i;
	MLToken* token;
	for( i=0; i<block->count; ++i ) {
		token = block->tokens + i;

		switch( token->type ) {
		case TK_LINE_COMMENT:
		case TK_BLOCK_COMMENT:
		case '{':
		case '}':
			//g_print("\n");
			break;
		}

		ch = token->buf[token->len];
		token->buf[token->len] = '\0';
		g_print("%s ", token->buf);
		token->buf[token->len] = ch;

		switch( token->type ) {
		case TK_LINE_COMMENT:
		case TK_BLOCK_COMMENT:
		case '{':
		case '}':
		case ';':
			//g_print("\n");
			break;
		}
	}
	g_print("\n--------------------------------------------------------\n");
}

#ifdef G_OS_WIN32
#include <windows.h>
#endif

gchar* cpp_parser_filename_to_filekey(const gchar* filename, glong namelen) {
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
		paths[0][0] = toupper(filename[0]);
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

void cpp_parser_init(CppParser* parser, gboolean enable_macro_replace) {
	parser->keywords_table = cpp_keywords_table_new();
	parser->enable_macro_replace = enable_macro_replace;
	parser->parsing_files = g_hash_table_new_full(g_str_hash, g_str_equal, 0, cpp_file_unref);
	parser->parsed_files  = g_hash_table_new_full(g_str_hash, g_str_equal, 0, cpp_file_unref);

	g_static_rw_lock_init( &(parser->include_paths_lock) );
	g_static_rw_lock_init( &(parser->files_lock) );
}

void cpp_parser_final(CppParser* parser) {
	cpp_keywords_table_free(parser->keywords_table);

	g_static_rw_lock_free( &(parser->files_lock) );
	g_static_rw_lock_free( &(parser->include_paths_lock) );
}

static CppFile* cpp_parser_do_parse_in_include_path(ParseEnv* env, const gchar* filename, glong namelen) {
	CppFile* file = 0;
	gchar* filekey;
	gchar* str;
	GList* p;

	for( p=env->parser->include_paths; !file && p; p=p->next ) {
		str = g_build_filename( (gchar*)(p->data), filename, 0 );
		if( str ) {
			filekey = cpp_parser_filename_to_filekey(str, -1);
			if( filekey )
				file = cpp_parser_parse_use_menv(env, filekey);
			g_free(str);
		}
	}

	return file;
}

CppFile* cpp_parser_find_parsed(CppParser* parser, const gchar* filekey) {
	CppFile* file;

	g_static_rw_lock_reader_lock( &(env->parser->files_lock) );
	file = (CppFile*)g_hash_table_lookup(parser->files, filekey);
	if( file && file->status==0 )
		cpp_file_ref(file);
	else
		file = 0;
	g_static_rw_lock_reader_unlock( &(env->parser->files_lock) );

	return file;
}

static void cpp_parser_do_parse(ParseEnv* env, gchar* buf, gsize len) {
	CppLexer lexer;
	Block block;
	TParseFn fn;
	BlockSpliter spliter;

	cpp_lexer_init(&lexer, buf, len, 1);
	env->lexer = &lexer;

	memset(&block, 0, sizeof(block));
	block.parent = &(env->file->root_scope);

	spliter_init(&spliter, env);

	while( (fn = spliter_next_block(&spliter, &block)) != 0 ) {
		// __dump_block(&block);
		fn(env, &block);
	}

	spliter_final(&spliter);
}

static gboolean load_convert_text(gchar** text, gsize* len, const gchar* charset, GError** err) {
	gsize bytes_written = 0;
	gchar* result = g_convert(*text, *len, "UTF-8", charset, 0, &bytes_written, err);
	if( result ) {
		if( g_utf8_validate(result, bytes_written, 0) ) {
			g_free(*text);
			*text = result;
			*len = bytes_written;
			return TRUE;
		}

		g_free(result);
	}

	return FALSE;
}

gboolean cpp_parser_load_file(const gchar* filename, gchar** text, gsize* len) {
	gchar** cs;
	const gchar* locale = 0;
	gchar* sbuf = 0;
	gsize  slen = 0;

	g_return_val_if_fail(filename && text && len , FALSE);
	g_return_val_if_fail(*filename, FALSE);

	if( !g_file_get_contents(filename, &sbuf, &slen, 0) )
		return FALSE;

	if( g_utf8_validate(sbuf, slen, 0) ) {
		*text = sbuf;
		*len = slen;
		return TRUE;
	}

	if( !g_get_charset(&locale) ) {		// get locale charset, and not UTF-8
		if( load_convert_text(&sbuf, &slen, locale, 0) ) {
			*text = sbuf;
			*len = slen;
			return TRUE;
		}
	}

	g_free(sbuf);
	return FALSE;
}

static CppFile* parse_include_file(ParseEnv* env, MLStr* filename, gboolean is_system_header) {
	CppFile* incfile = 0;
	gchar* filekey;
	gchar* path;
	gchar* str;

	//if( stopsign_is_set() )
	//	return;

	if( !is_system_header ) {
		if( g_path_is_absolute(filename->buf) ) {
			filekey = cpp_parser_filename_to_filekey(filename->buf, filename->len);

		} else {
			path = g_path_get_dirname(filename->buf);
			str = g_build_filename(path, filename->buf, 0);
			filekey = cpp_parser_filename_to_filekey(str, -1);
			g_free(str);
			g_free(path);
		}

		incfile = cpp_parser_parse_use_menv(env, filekey);
	}

	if( !incfile )
		incfile = cpp_parser_do_parse_in_include_path(env, filename->buf, filename->len);

	return incfile;
}

CppFile* cpp_parser_parse_use_menv(ParseEnv* env, const gchar* filekey) {
	CppFile* file = 0;
	CppFile* last;
	struct stat filestat;
	gchar* buf;
	gsize  len;

	if( !filekey )
		return file;

	// check already parsed and get file modify time
	// 
	memset(&filestat, 0, sizeof(filestat));
	if( g_stat(filekey, &filestat)!=0 )
		return 0;

	file = cpp_parser_find_parsed(env->parser, filekey);
	if( file && file->datetime!=filestat.st_mtime )
		file = 0;
	g_static_rw_lock_reader_unlock( &(env->parser->files_lock) );

	if( file )
		return file;

	// need parse file
	// 
	g_static_rw_lock_writer_lock( &(env->parser->files_lock) );

	file = (CppFile*)g_hash_table_lookup(env->parser->files, filekey);
	if( file && file->datetime==mtime ) {
		if( file->status==0 ) {	// parsed
			cpp_file_ref(file);

		} else {				// parsing
			if( file->status==(gint)env )
				file = 0;
			else
				cpp_file_ref(file);
		}

	} else {
		file = g_new0(CppFile, 1);
		file->datetime = filestat.st_mtime;
		file->status = (gint)env;
		file->ref_count = 1;
		file->filename = tiny_str_new(filekey, strlen(filekey));
		file->root_scope.type = CPP_ET_NCSCOPE;
		file->root_scope.file = file;

		g_hash_table_insert(env->parser->files, file->filename->buf, cpp_file_ref(file));
	}

	g_static_rw_lock_reader_unlock( &(env->parser->files_lock) );

	if( file ) {
		while( file->status!=0 )
			g_usleep(500);
		return file;
	}

	if( cpp_parser_load_file(env, filekey, &buf, &len) ) {
		last = env->file;
		env->file = file;

		// create macro environ if need
		if( env->parser->enable_macro_replace ) {
			env->rmacros_table = g_hash_table_new_full(g_str_hash, g_str_equal, 0, (GDestroyNotify)rmacro_free);
			env->parse_include_file = parse_include_file;
		}

		// start parse
		cpp_parser_do_parse(env, buf, len);

		env->file = last;
	}

	file->status = 0;
	return file;
}

CppFile* cpp_parser_parse(CppParser* parser, const gchar* filekey) {
	ParseEnv env = { parser, 0, 0, 0 };
	CppFile* file = cpp_parser_parse_use_menv(&env, filekey);

	if( env.rmacros_table )
		g_hash_table_destroy(env->rmacros_table);

	return file;
}

