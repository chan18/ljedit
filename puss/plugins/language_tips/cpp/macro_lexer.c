// mlexer.c
// 

#include "macro_lexer.h"

#include <memory.h>
#include <string.h>
#include <assert.h>

static inline void mlstr_cpy(MLStr* dst, MLStr* src) {
	if( src->buf ) {
		dst->buf = g_slice_alloc(src->len + 1);
		dst->len = src->len;
		memcpy(dst->buf, src->buf, dst->len);
		dst->buf[dst->len] = '\0';
	}
}

static void ml_str_join(MLStr* out, MLStr* array, gint count) {
	gint i;
	gchar* p;
	gsize len = 0;
	for( i=0; i<count; ++i )
		len += 1 + array[i].len;

	if( len==0 )
		return;

	len = (out->buf) ? (out->len + len) : (len - 1);

	p = g_slice_alloc(len + 1);
	if( out->buf ) {
		memcpy(p, out->buf, out->len);
		g_slice_free1(out->len + 1, out->buf);
		out->buf = p;
		p += out->len;
		out->len = len;
		i = 0;

	} else {
		out->buf = p;
		out->len = len;
		memcpy(p, array[0].buf, array[0].len);
		p += array[0].len;
		i = 1;
	}

	for( ; i<count; ++i ) {
		*p = ' ';
		++p;
		memcpy(p, array[i].buf, array[i].len);
		p += array[i].len;
	}

	assert( p==(out->buf + out->len) );
	*p = '\0';
}

/*
#define FRAME_HAS_NEXT()	(frame->ps < frame->pe)
#define FRAME_PREV_CH()		frame->ps = g_utf8_prev_char(frame->ps)
#define FRAME_NEXT_CH()		frame->ps = g_utf8_next_char(frame->ps)
#define FRAME_GET_CH()		g_utf8_get_char(frame->ps)
*/

#define FRAME_HAS_NEXT()	(frame->ps < frame->pe)
#define FRAME_PREV_CH()		--frame->ps
#define FRAME_NEXT_CH()		++frame->ps
#define FRAME_GET_CH()		*(frame->ps)

#define CPP_LEXER_NEXT_NOCOMMENT(lexer, token) \
	do { \
		cpp_lexer_next(lexer, token); \
	} while( ((token)->type==TK_LINE_COMMENT) || ((token)->type==TK_BLOCK_COMMENT) )

#define LINE_FRAME_SKIP_WS() \
	while( FRAME_HAS_NEXT() ) { \
		ch = FRAME_GET_CH(); \
		if( ch==' ' || ch=='\t' ) \
			FRAME_NEXT_CH(); \
		else \
			break; \
	}

#define MACRO_ARGS_MAX 256

static CppElem* find_macro(ParseEnv* env, MLStr* name) {
	CppElem* res;
	gchar ch = name->buf[name->len];
	name->buf[name->len] = '\0';
	res = g_hash_table_lookup(env->rmacros_table, name->buf);
	name->buf[name->len] = ch;
	return res;
}

static void insert_rmacros_into_env(ParseEnv* env, CppFile* file) {
	GList* p;
	CppElem* elem;
	CppFile* incfile;

	if( g_hash_table_lookup(env->used_files, file) )
		return;

	g_hash_table_insert(env->used_files, cpp_file_ref(file), file);

	for( p=file->root_scope.v_ncscope.scope; p; p=p->next ) {
		elem = (CppElem*)(p->data);
		switch( elem->type ) {
		case CPP_ET_MACRO:
			g_hash_table_replace(env->rmacros_table, elem->name->buf, elem);
			break;

		case CPP_ET_UNDEF:
			g_hash_table_remove(env->rmacros_table, elem->name->buf);
			break;

		case CPP_ET_INCLUDE:
			incfile = cpp_parser_find_parsed(env->parser, elem->v_include.include_file);
			if( incfile ) {
				insert_rmacros_into_env(env, incfile);
				cpp_file_unref(incfile);
			}
			break;
		}
	}
}

static void on_macro_define(ParseEnv* env, gint line, MLStr* name, gint argc, MLStr* argv, MLStr* value, MLStr* comment, MLStr* desc) {
	CppElem* elem;
	gint i;

	elem = cpp_elem_new();
	elem->type = CPP_ET_MACRO;
	elem->file = env->file;
	elem->name = tiny_str_new(name->buf, name->len);
	elem->sline = line;
	elem->eline = line;
	elem->decl = tiny_str_new(desc->buf, desc->len);

	elem->v_define.argc = argc;
	if( argc > 0 ) {
		elem->v_define.argv = g_slice_alloc(sizeof(gpointer) * argc);
		for( i=0; i<argc; ++i )
			elem->v_define.argv[i] = tiny_str_new(argv[i].buf, argv[i].len);
	}

	if( value )
		elem->v_define.value = tiny_str_new(value->buf, value->len);

	cpp_scope_insert(&(env->file->root_scope), elem);

	g_hash_table_replace(env->rmacros_table, elem->name->buf, elem);
}

static void on_macro_undef(ParseEnv* env, gint line, MLStr* name) {
	CppElem* elem;
	
	elem = cpp_elem_new();
	elem->type = CPP_ET_UNDEF;
	elem->file = env->file;
	elem->name = tiny_str_new(name->buf, name->len);
	elem->sline = line;
	elem->eline = line;
	elem->decl = tiny_str_new(0, 7 + name->len);
	memcpy(elem->decl->buf, "#undef ", 7);
	memcpy(elem->decl->buf+7, name->buf, name->len);

	cpp_scope_insert(&(env->file->root_scope), elem);

	g_hash_table_remove(env->rmacros_table, name->buf);
}

static void on_macro_include(ParseEnv* env, MLStr* filename, gboolean is_system_header, gint line) {
	CppFile* incfile = 0;
	gchar* str;
	CppElem* elem;

	//if( stopsign_is_set() )
	//	return;

	elem = cpp_elem_new();
	elem->type = CPP_ET_INCLUDE;
	elem->file = env->file;
	elem->name = tiny_str_new("_include_", 9);
	elem->sline = line;
	elem->eline = line;
	elem->v_include.filename = tiny_str_new(filename->buf, filename->len);
	elem->v_include.sys_header = is_system_header;

	str = g_strdup_printf("#include %c%s%c", is_system_header?'<':'\"', elem->v_include.filename->buf, is_system_header?'<':'\"');
	elem->decl = tiny_str_new(str, (gsize)strlen(str));
	g_free(str);

	cpp_scope_insert( &(env->file->root_scope), elem );

	/*
	line = filename->buf[filename->len];
	filename->buf[filename->len] = 0;
		g_print("%s : %d\n", filename->buf, memcmp(filename, "glib/gmem.h", 11));
	filename->buf[filename->len] = line;
	*/
	incfile = parse_include_file(env, filename, is_system_header);

	if( incfile ) {
		elem->v_include.include_file = g_strdup(incfile->filename->buf);
		insert_rmacros_into_env(env, incfile);
		cpp_file_unref(incfile);
	}
}

static void cpp_parse_macro(ParseEnv* env, CppLexer* lexer) {
	gint ch = 0;
	CppFrame* frame;
	MLToken token;
	MLStr desc;

	frame = lexer->stack + lexer->top;
	frame->is_new_line = FALSE;
	desc.buf = frame->ps;
	desc.len = (frame->pe - frame->ps);

	FRAME_NEXT_CH();
	CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
	if( token.type!=TK_ID )
		return;

	if( token.len==6 && memcmp(token.buf, "define", 6)==0 ) {
		gint line = token.line;
		MLStr name = {0, 0};
		gint  argc = -1;
		MLStr argv[MACRO_ARGS_MAX];
		MLStr value = {0, 0};
		MLStr comment = {0, 0};

		// parse name
		CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
		if( token.type!=TK_ID )
			return;
		name.buf = token.buf;
		name.len = token.len;

		// parse args
		if( FRAME_HAS_NEXT() ) {
			ch = FRAME_GET_CH();
			if( ch=='(' ) {
				argc = 0;
				FRAME_NEXT_CH();
				CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
				for(;;) {
					if( token.type==')' )
						break;

					if( token.type!=TK_ID && token.type!=SG_ELLIPSIS )
						return;	// error : bad arg

					if( argc >= MACRO_ARGS_MAX )
						return;	// error : too many args

					if( (argc > 0) && argv[argc-1].buf[0]=='.' )
						return;	// error : ... must the last arg

					argv[argc].buf = token.buf;
					argv[argc].len = token.len;
					++argc;

					CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
					if( token.type==',' )
						CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
				}
			}
		}

		// parse value

		LINE_FRAME_SKIP_WS();
		if( FRAME_HAS_NEXT() ) {
			while( token.type != TK_EOF ) {
				value.buf = frame->ps;
				do {
					value.len = (gsize)(frame->ps - value.buf);
					cpp_lexer_next(lexer, &token);
					switch(token.type) {
					case TK_LINE_COMMENT:
						token.type = TK_EOF;
						comment.buf = token.buf;
						comment.len = token.len;
						break;
					case TK_BLOCK_COMMENT:
						cpp_lexer_next(lexer, &token);
						if( token.type==TK_EOF ) {
							comment.buf = token.buf;
							comment.len = token.len;
						}
						break;
					default:
						break;
					}
				} while( token.type != TK_EOF );

				if( value.len==0 )
					value.buf = 0;
			}
		}

		on_macro_define(env, line, &name, argc, argv, &value, &comment, &desc);

	} else if( token.len==5 && memcmp(token.buf, "undef", 5)==0 ) {
		MLStr name;
		CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
		if( token.type==TK_ID ) {
			name.buf = token.buf;
			name.len = token.len;

			on_macro_undef(env, token.line, &name);
		}

	} else if( token.len==7 && memcmp(token.buf, "include", 7)==0 ) {
		gchar ss, es;
		MLStr filename;

		while( FRAME_HAS_NEXT() ) {
			ch = FRAME_GET_CH();
			FRAME_NEXT_CH();
			if( ch=='<' || ch=='"' )
				break;
		}

		if( ch!='<' && ch!='"' )
			return;

		ss = ch;
		es = (ss=='<') ? '>' : '"';

		filename.buf = frame->ps;
		while( FRAME_HAS_NEXT() ) {
			ch = FRAME_GET_CH();
			if( ch==es ) {
				filename.len = (gsize)(frame->ps - filename.buf);
				if( filename.len )
					on_macro_include(env, &filename, ss=='<', token.line);
				break;
			}
			FRAME_NEXT_CH();
		}
	}
}

typedef struct {
	MLStr		str;
	gboolean	is_owner;
} MLArg;

static gboolean cpp_parse_macro_args(CppLexer* lexer, MLToken* token, gint* argc, MLArg argv[MACRO_ARGS_MAX]) {
	#define WORDS_MAX 256

	gint layer;
	gint count;
	MLArg* arg;
	MLStr words[WORDS_MAX];

	CPP_LEXER_NEXT_NOCOMMENT(lexer, token);
	if( token->type!='(' )
		return FALSE;

	*argc = 0;
	while( token->type!=')' ) {
		if( *argc >= MACRO_ARGS_MAX )
			return FALSE;

		arg = argv + *argc;
		layer = 0;
		count = 0;

		for(;;) {
			arg->is_owner = FALSE;
			arg->str.buf = 0;
			arg->str.len = 0;

			CPP_LEXER_NEXT_NOCOMMENT(lexer, token);
			if( token->type==TK_EOF )
				return FALSE;

			if( token->type==')' ) {
				if( layer==0 )
					break;
				--layer;

			} else if( token->type==',' ) {
				if( layer==0 )
					break;

			} else if( token->type=='(' ) {
				++layer;

			} else if( token->type==';' || token->type=='{' || token->type=='}' ) {
				return FALSE;
			}

			if( count==WORDS_MAX ) {
				ml_str_join( &(arg->str), words, count );
				count = 0;
			}

			words[count].buf = token->buf;
			words[count].len = token->len;
			++count;
		}

		if( count > 0 ) {
			if( (count > 1) || arg->str.buf ) {
				ml_str_join( &(arg->str), words, count );
				arg->is_owner = TRUE;
			} else {
				arg->str = words[0];
				arg->is_owner = FALSE;
			}

			++(*argc);
		}
	}

	return TRUE;
}

static gint get_macro_arg_pos(CppElem* macro, MLToken* token) {
	gint i;
	if( (token->type==TK_ID) && (macro->v_define.argc > 0) )
		for( i=0; i<macro->v_define.argc; ++i )
			if( (token->len==tiny_str_len(macro->v_define.argv[i])) && (memcmp(token->buf, macro->v_define.argv[i]->buf, token->len)==0) )
				return i;
	return -1;
}

static void do_macro_replace(CppElem* macro, CppLexer* lexer, gint argc, MLArg argv[MACRO_ARGS_MAX]) {

#define MACRO_REPLACE_BUFFER_MAX 64*1024

	gchar sbuf[MACRO_REPLACE_BUFFER_MAX];
	CppMacroDefine* def = &(macro->v_define);
	gchar* pd = sbuf;
	MLToken token;
	CppLexer rlexer;
	guint i;
	gint pos;
	gint need_space_size;

	cpp_lexer_init(&rlexer, def->value->buf, tiny_str_len(def->value), 0);
	rlexer.stack[0].is_new_line = FALSE;

	do {
		CPP_LEXER_NEXT_NOCOMMENT(&rlexer, &token);
		if( token.type=='#' ) {
			CPP_LEXER_NEXT_NOCOMMENT(&rlexer, &token);
			pos = get_macro_arg_pos(macro, &token);

			if( (pos >= 0) && (pos < argc)
				&& (argv[pos].str.len > 0)
				&& (pd + (1 + argv[pos].str.len + 1) < sbuf + MACRO_REPLACE_BUFFER_MAX) )
			{
				*pd++ = '"';
				for( i=0; i<tiny_str_len(def->argv[pos]); ++i )
					*pd++ = def->argv[pos]->buf[i];
				*pd++ = '"';
				continue;
			}
			break;
		}

		if( token.type==SG_DBL_SHARP ) {
			CPP_LEXER_NEXT_NOCOMMENT(&rlexer, &token);
			need_space_size = 0;
		} else {
			need_space_size = 1;
		}

		pos = get_macro_arg_pos(macro, &token);
		if( (pos >= 0) && (pos < argc) ) {
			if( (argv[pos].str.len > 0) && (pd + (need_space_size + argv[pos].str.len) < sbuf + MACRO_REPLACE_BUFFER_MAX) ) {
				*pd++ = ' ';
				for( i=0; i<argv[pos].str.len; ++i )
					*pd++ = argv[pos].str.buf[i];
				continue;
			}
			break;

		} else if( token.len > 0 ) {
			if( pd + (need_space_size + token.len) < sbuf + MACRO_REPLACE_BUFFER_MAX ) {
				for( i=0; i<need_space_size; ++i )
					*pd++ = ' ';
				for( i=0; i<(gint)token.len; ++i )
					*pd++ = token.buf[i];
				continue;
			}
			break;

		} else {
			break;
		}

	} while( token.type != TK_EOF );

	if( token.type==TK_EOF ) {
		*pd = '\0';
		if( pd > sbuf ) {
			MLStr str = { sbuf, (gsize)(pd - sbuf) };
			cpp_lexer_frame_push(lexer, &str, TRUE, FALSE, macro);
		}
	}

	cpp_lexer_final(&rlexer);
}

static gboolean macro_replace(ParseEnv* env, CppElem* macro, MLToken* token) {
	CppMacroDefine* def = &(macro->v_define);
	// check in_use
	gint i;
	for( i=0; i<=env->lexer->top; ++i )
		if( env->lexer->stack[i].tag==macro )
			return FALSE;

	if( def->argc < 0 ) {
		if( def->value ) {
			MLStr value = { def->value->buf, tiny_str_len(def->value) };
			cpp_lexer_frame_push(env->lexer, &value, TRUE, FALSE, macro);
		}

	} else {
		gint argc = -1;
		MLArg argv[MACRO_ARGS_MAX];
		gboolean res = cpp_parse_macro_args(env->lexer, token, &argc, argv);
		if( res ) {
			if( def->value ) {
				if( def->argc==0 ) {
					MLStr value = { def->value->buf, tiny_str_len(def->value) };
					cpp_lexer_frame_push(env->lexer, &value, TRUE, FALSE, macro);
				} else {
					do_macro_replace(macro, env->lexer, argc, argv);
				}
			}
		}

		for( i=0; i<argc; ++i) {
			if( argv[i].is_owner )
				g_slice_free1(argv[i].str.len + 1, argv[i].str.buf);
		}

		return res;
 
	}

	return TRUE;
}

void cpp_macro_lexer_init(ParseEnv* env) {
	if( env->parser->enable_macro_replace ) {
		env->rmacros_table = g_hash_table_new(g_str_hash, g_str_equal);
		env->used_files = g_hash_table_new_full(g_direct_hash, g_direct_equal, (GDestroyNotify)cpp_file_unref, 0);
		env->include_paths = cpp_parser_include_paths_ref(env->parser);
	}
}

void cpp_macro_lexer_final(ParseEnv* env) {
	if( env->rmacros_table )
		g_hash_table_destroy(env->rmacros_table);
	if( env->used_files )
		g_hash_table_destroy(env->used_files);
	cpp_parser_include_paths_unref(env->include_paths);
}

void cpp_macro_lexer_next(ParseEnv* env, MLToken* token) {
	if( !env ) {
		cpp_lexer_next(env->lexer, token);
		return;
	}

	for(;;) {
		cpp_lexer_next(env->lexer, token);
		if( token->type==TK_MACRO ) {
			CppLexer mlexer;

			cpp_lexer_init(&mlexer, token->buf, token->len, token->line);
			cpp_parse_macro(env, &mlexer);
			cpp_lexer_final(&mlexer);
			continue;
		}

		if( token->type==TK_ID ) {
			MLStr name = { token->buf, token->len };
			CppElem* macro = find_macro(env, &name);

			if( !macro )
				break;

			if( macro_replace(env, macro, token) )
				continue;
		}

		break;
	}
}

void cpp_macro_lexer_insert_rmacros_into_env(ParseEnv* env, CppFile* file) {
	insert_rmacros_into_env(env, file);
}

