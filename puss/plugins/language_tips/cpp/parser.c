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

#ifdef _DEBUG
	static void trace_parser_status(const gchar* ms) {
		g_printerr("DEBUG[csp_debug %s] : files:%d elems:%d str:%d\n", ms, cps_debug_file_count, cps_debug_elem_count, cps_debug_tinystr_count);
	}
#else
	#define trace_parser_status(ms)
#endif

static void cpp_parser_on_remove_file(gchar* filekey, CppFile* file, CppParser* self) {
	if( self->cb_file_remove )
		self->cb_file_remove(file, self->cb_tag);
	cpp_file_unref(file);
}

void cpp_parser_init(CppParser* self, gboolean enable_macro_replace) {
	self->keywords_table = cpp_keywords_table_new();
	self->enable_macro_replace = enable_macro_replace;
	self->files  = g_hash_table_new(g_str_hash, g_str_equal);

	g_static_rw_lock_init( &(self->include_paths_lock) );
	g_static_rw_lock_init( &(self->files_lock) );

	//trace_parser_status("init");
}

void cpp_parser_final(CppParser* self) {
	//trace_parser_status("clean_start");
	g_hash_table_foreach(self->files, cpp_parser_on_remove_file, self);
	trace_parser_status("  clean_end");

	g_hash_table_destroy(self->files);
	cpp_keywords_table_free(self->keywords_table);

	cpp_parser_include_paths_unref(self->include_paths);

	g_static_rw_lock_free( &(self->files_lock) );
	g_static_rw_lock_free( &(self->include_paths_lock) );
}

void cpp_parser_include_paths_set(CppParser* self, GList* paths) {
	CppIncludePaths* include_paths;
	CppIncludePaths* old;

	include_paths = g_new(CppIncludePaths, 1);
	include_paths->path_list = paths;
	include_paths->ref_count = 1;

	g_static_rw_lock_writer_lock( &(self->include_paths_lock) );
	old = self->include_paths;
	self->include_paths = include_paths;
	g_static_rw_lock_writer_unlock( &(self->include_paths_lock) );

	if( old && g_atomic_int_dec_and_test(&(old->ref_count)) ) {
		g_list_foreach(old->path_list, g_free, 0);
		g_list_free(old->path_list);
		g_free(old);
	}
}

CppIncludePaths* cpp_parser_include_paths_ref(CppParser* self) {
	CppIncludePaths* paths;
	g_static_rw_lock_reader_lock( &(self->include_paths_lock) );
	paths = self->include_paths;
	if( paths )
		g_atomic_int_inc( &(paths->ref_count) );
	g_static_rw_lock_reader_unlock( &(self->include_paths_lock) );

	return paths;
}

void cpp_parser_include_paths_unref(CppIncludePaths* paths) {
	if( paths && g_atomic_int_dec_and_test(&(paths->ref_count)) ) {
		g_list_foreach(paths->path_list, g_free, 0);
		g_list_free(paths->path_list);
		g_free(paths);
	}
}

CppFile* cpp_parser_find_parsed(CppParser* self, const gchar* filekey) {
	CppFile* file;

	if( !filekey )
		return 0;

	g_static_rw_lock_reader_lock( &(self->files_lock) );
	file = (CppFile*)g_hash_table_lookup(self->files, filekey);
	if( file && file->status==0 )
		cpp_file_ref(file);
	else
		file = 0;
	g_static_rw_lock_reader_unlock( &(self->files_lock) );

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
	cpp_lexer_final(&lexer);
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

static CppFile* cpp_parser_parse_use_menv(ParseEnv* env, const gchar* filekey, gboolean force_rebuild) {
	CppFile* file = 0;
	CppFile* last_file;
	CppLexer* last_lexer;
	struct stat filestat;
	gchar* buf;
	gsize  len;

	if( !filekey )
		return file;

	// check already parsed and get file modify time
	// 
	memset(&filestat, 0, sizeof(filestat));

	// TODO : g_stat a/m/c time error, maybe a bug in win32, I need try in linux!!!
	// 
	//if( g_stat(filekey, &filestat)!=0 )
	if( stat(filekey, &filestat)!=0 )
		return file;

	// need parse file
	// 
	g_static_rw_lock_writer_lock( &(env->parser->files_lock) );

	file = (CppFile*)g_hash_table_lookup(env->parser->files, filekey);

	if( file && file->status!=0 ) {
		// parsing
		if( file->status==(gint)env )
			file = 0;
		else
			cpp_file_ref(file);

	} else {
		// parsed
		if( file && file->datetime==filestat.st_mtime && !force_rebuild ) {
			cpp_file_ref(file);

		} else {
			if( file ) {
				g_hash_table_remove(env->parser->files, file->filename);
				cpp_parser_on_remove_file(file->filename, file, env->parser);
			}

			file = g_slice_new0(CppFile);
			DEBUG_FILE_INC();
			file->datetime = filestat.st_mtime;
			file->status = (gint)env;
			file->ref_count = 1;
			file->filename = tiny_str_new(filekey, strlen(filekey));
			file->root_scope.type = CPP_ET_NCSCOPE;
			file->root_scope.file = file;

			g_hash_table_insert(env->parser->files, file->filename->buf, cpp_file_ref(file));
			g_hash_table_insert(env->used_files, cpp_file_ref(file), 0);
		}
	}

	g_static_rw_lock_writer_unlock( &(env->parser->files_lock) );

	if( !file )
		return file;

	if( file->status!=(gint)env ) {
		while( file->status!=0 )
			g_usleep(500);

	} else {
		if( cpp_parser_load_file(filekey, &buf, &len) ) {
			last_file = env->file;
			last_lexer = env->lexer;

			env->file = file;
			env->lexer = 0;

			// start parse
			cpp_parser_do_parse(env, buf, len);
			g_free(buf);

			env->file = last_file;
			env->lexer = last_lexer;
		}

		file->status = 0;
		if( env->parser->cb_file_insert )
			(*(env->parser->cb_file_insert))(file, env->parser->cb_tag);
	}

	return file;
}

static CppFile* cpp_parser_do_parse_in_include_path(ParseEnv* env, const gchar* filename) {
	CppFile* file = 0;
	gchar* filekey;
	gchar* str;
	GList* p;

	for( p=env->include_paths->path_list; !file && p; p=p->next ) {
		str = g_build_filename( (gchar*)(p->data), filename, 0 );
		if( str ) {
			filekey = cpp_filename_to_filekey(str, -1);
			if( filekey )
				file = cpp_parser_parse_use_menv(env, filekey, FALSE);
			g_free(str);
		}
	}

	return file;
}

CppFile* parse_include_file(ParseEnv* env, MLStr* filename, gboolean is_system_header) {
	CppFile* incfile = 0;
	gchar* filekey;
	gchar* path;
	gchar* str;
	gchar ch;

	ch = filename->buf[filename->len];
	filename->buf[filename->len] = '\0';

	//if( stopsign_is_set() )
	//	return;

	if( !is_system_header ) {
		if( g_path_is_absolute(filename->buf) ) {
			filekey = cpp_filename_to_filekey(filename->buf, filename->len);

		} else {
			path = g_path_get_dirname(env->file->filename->buf);
			str = g_build_filename(path, filename->buf, 0);
			filekey = cpp_filename_to_filekey(str, -1);
			g_free(str);
			g_free(path);
		}

		incfile = cpp_parser_parse_use_menv(env, filekey, FALSE);
	}

	if( !incfile && env->include_paths )
		incfile = cpp_parser_do_parse_in_include_path(env, filename->buf);

	filename->buf[filename->len] = ch;

	return incfile;
}

CppFile* cpp_parser_parse(CppParser* self, const gchar* filekey, gboolean force_rebuild) {
	ParseEnv env;
	CppFile* file;

	memset(&env, 0, sizeof(ParseEnv));

	env.parser = self;

	cpp_macro_lexer_init(&env);

	file = cpp_parser_parse_use_menv(&env, filekey, force_rebuild);

	cpp_macro_lexer_final(&env);

	return file;
}

