// parser.c
// 

#include "parser.h"
#include <sys/stat.h>

#include "keywords.h"
#include "cps.h"


static CppFile* cpp_parser_do_parse_in_include_path(CppParser* env, const gchar* filename, glong namelen);


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

static void on_macro_include(MLStr* filename, gboolean is_system_header, gint line, BlockTag* tag) {
	CppFile* incfile = 0;
	gchar* filekey = 0;
	gchar* path;
	gchar* str;
	CppElem* elem;

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

		incfile = cpp_parser_parse_use_menv(tag->env, tag->menv, filekey, -1);
	}

	if( !incfile )
		incfile = cpp_parser_do_parse_in_include_path(tag->env, filename->buf, filename->len);

	if( incfile ) {
		elem = cpp_elem_new();
		elem->type = CPP_ET_INCLUDE;
		elem->name = tiny_str_new("_include_", -1);
		elem->sline = line;
		elem->eline = line;
		elem->v_include.filename = tiny_str_new(filename->buf, filename->len);
		elem->v_include.sys_header = is_system_header;
		elem->v_include.include_file = tiny_str_copy(incfile->filename);

		str = g_strdup_printf("#include %c%s%c", is_system_header?'<':'\"', filename->buf, is_system_header?'<':'\"');
		elem->decl = tiny_str_new(str, strlen(str));
		g_free(str);

		cpp_scope_insert( &(tag->file->root_scope), elem );
	}
}

void cpp_parser_init(CppParser* parser, gboolean enable_macro_replace) {
	parser->keywords_table = cpp_keywords_table_new();
	parser->enable_macro_replace = enable_macro_replace;
	parser->parsing_files = g_hash_table_new_full(g_str_hash, g_str_equal, 0, cpp_file_unref);
	parser->parsed_files  = g_hash_table_new_full(g_str_hash, g_str_equal, 0, cpp_file_unref);
}

void cpp_parser_final(CppParser* parser) {
	cpp_keywords_table_free(parser->keywords_table);
}

static CppFile* cpp_parser_do_parse_in_include_path(CppParser* parser, const gchar* filename, glong namelen) {
	CppFile* file = 0;
	gchar* filekey;
	gchar* str;
	GList* p;

	for( p=parser->include_paths; !file && p; p=p->next ) {
		str = g_build_filename( (gchar*)(p->data), filename, 0 );
		if( str ) {
			filekey = cpp_parser_filename_to_filekey(str, -1);
			if( filekey )
				file = cpp_parser_parse(env, filekey, -1);
			g_free(str);
		}
	}

	return file;
}

static CppFile* cpp_parser_check_parsed(CppParser* parser, const gchar* filekey, time_t* mtime) {
	CppFile* file;
	struct stat filestat;

	memset(&filestat, 0, sizeof(filestat));
	if( g_stat(filekey, &filestat)!=0 )
		return 0;

	*mtime = filestat.st_mtime;
	if( (file = (CppFile*)g_hash_table_lookup(parser->parsed_files, filekey)) != 0 )
		if( file->datetime != *mtime )
			file = 0;

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

CppFile* cpp_parser_parse_use_menv(ParseEnv* env, const gchar* filekey, glong keylen) {
	CppFile* file = 0;
	time_t mtime = 0;
	gchar* buf;
	gsize  len;

	if( !filekey )
		return file;

	// check re-parsing
	file = (CppFile*)g_hash_table_lookup(env->parser->parsing_files, filekey);
	if( file )
		return file;

	// check already parsed and get file modify time
	// 
	file = cpp_parser_check_parsed(env->parser, filekey, &mtime);
	if( file )
		return cpp_file_ref(file);

	if( !cpp_parser_load_file(env, filekey, &buf, &len) )
		return 0;

	// need parse file
	file = g_new0(CppFile, 1);
	file->datetime = mtime;
	file->ref_count = 1;
	file->filename = tiny_str_new(filekey, keylen==-1 ? strlen(filekey) : (gshort)keylen);
	file->root_scope.type = CPP_ET_NCSCOPE;
	file->root_scope.file = file;
	env->file = file;

	g_hash_table_insert(env->parser->parsing_files, file->filename->buf, cpp_file_ref(file));

	// create macro environ if need
	if( env->parser->enable_macro_replace ) {
		env->rmacros_table = g_hash_table_new_full(g_str_hash, g_str_equal, 0, (GDestroyNotify)rmacro_free);
		//menv.macro_environ.on_macro_include = (TOnMacroIncludeFn)on_macro_include;
	}

	// start parse
	cpp_parser_do_parse(env, buf, len);
	g_hash_table_insert(env->parser->parsed_files, file->filename->buf, cpp_file_ref(file));

	g_hash_table_remove(env->parser->parsing_files, file->filename->buf);

	return file;
}

CppFile* cpp_parser_parse(CppParser* parser, const gchar* filekey, glong keylen) {
	ParseEnv env = { parser, 0, 0, 0 };
	CppFile* file = cpp_parser_parse_use_menv(&env, filekey, keylen);

	if( env.rmacros_table )
		g_hash_table_destroy(env->rmacros_table);

	return file;
}

// utils

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


