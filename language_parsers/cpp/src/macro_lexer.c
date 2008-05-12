// mlexer.c
// 

#include "macro_lexer.h"

#include <memory.h>


static void ml_str_join(MLStr* out, MLStr* array, gint count) {
	gint i;
	gchar* p;
	gsize len = 0;
	for( i=0; i<count; ++i )
		len += 1 + array[i].len;

	if( len==0 )
		return;

	if( out->len )
		len = out->len + len + 1;

	p = g_new(gchar, len);
	if( out->buf ) {
		memcpy(p, out->buf, out->len);
		g_free(out->buf);
		out->buf = p;
		p += out->len;
		out->len = len - 1;
		i = 0;

	} else {
		out->buf = p;
		out->len = len - 1;
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

	g_assert( p==(out->buf + out->len) );
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

static void cpp_parse_macro(MacroEnviron* env, CppLexer* lexer, gpointer tag) {
	gint ch;
	CppFrame* frame;
	MLToken token;

	frame = lexer->stack + lexer->top;
	frame->is_new_line = FALSE;

	CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
	if( token.type!=TK_ID )
		return;

	if( token.len==6 && memcmp(token.buf, "define", 6)==0 ) {
		MLStr name = { 0, 0 };
		gint  argc = -1;
		MLStr argv[MACRO_ARGS_MAX];
		MLStr value = { 0, 0 };
		MLStr comment = { 0, 0 };

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

		(*(env->on_macro_define))(&name, argc, argv, &value, &comment, tag);

	} else if( token.len==7 && memcmp(token.buf, "include", 7)==0 ) {
		gchar ss, es;
		MLStr filename;

		CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
		if( token.type!='<' && token.type!='"' )
			return;
		ss = token.buf[0];
		es = (ss=='<') ? '>' : '"';

		filename.buf = frame->ps;
		while( FRAME_HAS_NEXT() ) {
			ch = FRAME_GET_CH();
			if( ch==es ) {
				filename.len = (gsize)(frame->ps - filename.buf);
				if( filename.len )
					(*(env->on_macro_include))(&filename, ch=='<', tag);
				break;
			}
			FRAME_NEXT_CH();
		}

	} else if( token.len==5 && memcmp(token.buf, "undef", 5)==0 ) {
		MLStr name;
		CPP_LEXER_NEXT_NOCOMMENT(lexer, &token);
		if( token.type==TK_ID ) {
			name.buf = token.buf;
			name.len = token.len;

			(*(env->on_macro_undef))(&name, tag);
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

static gint get_macro_arg_pos(RMacro* macro, MLToken* token) {
	gint i;
	if( (token->type==TK_ID) && (macro->argc > 0) )
		for( i=0; i<macro->argc; ++i )
			if( (token->len==macro->argv[i].len) && (memcmp(token->buf, macro->argv[i].buf, token->len)==0) )
				return i;
	return -1;
}

static void do_macro_replace(RMacro* macro, CppLexer* lexer, gint argc, MLArg argv[MACRO_ARGS_MAX]) {

#define MACRO_REPLACE_BUFFER_MAX 64*1024

	gchar sbuf[MACRO_REPLACE_BUFFER_MAX];
	gchar* pd = sbuf;
	MLToken token;
	CppLexer rlexer;
	gint i;
	gint pos;

	cpp_lexer_init(&rlexer, macro->value.buf, macro->value.len, 0);

	do {
		CPP_LEXER_NEXT_NOCOMMENT(&rlexer, &token);
		if( token.type=='#' ) {
			CPP_LEXER_NEXT_NOCOMMENT(&rlexer, &token);
			if( (pos = get_macro_arg_pos(macro, &token)) >= 0 ) {
				if( (pos < argc) && argv[pos].str.len > 0 ) {
					if( pd + (1 + argv[pos].str.len + 1) < sbuf + MACRO_REPLACE_BUFFER_MAX ) {
						*pd++ = '"';
						for( i=0; i<(gint)macro->argv[pos].len; ++i )
							*pd++ = macro->argv[pos].buf[i];
						*pd++ = '"';
						continue;
					}
				}
			}
			break;

		} else if( token.type==SG_DBL_SHARP ) {
			CPP_LEXER_NEXT_NOCOMMENT(&rlexer, &token);
			if( (pos = get_macro_arg_pos(macro, &token)) >= 0 ) {
				if( (pos < argc) && argv[pos].str.len > 0 ) {
					if( pd + argv[pos].str.len < sbuf + MACRO_REPLACE_BUFFER_MAX ) {
						for( i=0; i<(gint)macro->argv[pos].len; ++i )
							*pd++ = macro->argv[pos].buf[i];
						continue;
					}
				}
			}
			break;

		} else if( (pos = get_macro_arg_pos(macro, &token)) >= 0 ) {
			if( (pos < argc) && argv[pos].str.len > 0 ) {
				if( pd + (1 + argv[pos].str.len + 1) < sbuf + MACRO_REPLACE_BUFFER_MAX ) {
					*pd++ = ' ';
					for( i=0; i<(gint)macro->argv[pos].len; ++i )
						*pd++ = macro->argv[pos].buf[i];
					continue;
				}
			}
			break;

		} else if( (pos < argc) && token.len > 0 ) {
			if( pd + (1 + token.len) < sbuf + MACRO_REPLACE_BUFFER_MAX ) {
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

static gboolean macro_replace(MacroEnviron* env, RMacro* macro, CppLexer* lexer, MLToken* token) {
	// check in_use
	gint i;
	for( i=0; i<=lexer->top; ++i )
		if( lexer->stack[i].tag==macro )
			return FALSE;

	if( macro->argc < 0 ) {
		if( macro->value.len )
			cpp_lexer_frame_push(lexer, &(macro->value), TRUE, FALSE, macro);

	} else {
		gint argc = -1;
		MLArg argv[MACRO_ARGS_MAX];
		gboolean res = cpp_parse_macro_args(lexer, token, &argc, argv);
		if( res ) {
			if( macro->value.len ) {
				if( macro->argc==0 )
					cpp_lexer_frame_push(lexer, &(macro->value), TRUE, FALSE, macro);
				else
					do_macro_replace(macro, lexer, argc, argv);
			}
		}

		for( i=0; i<argc; ++i)
			if( argv[i].is_owner )
				g_free(argv[i].str.buf);

		return res;
 
	}

	return TRUE;
}

void cpp_macro_lexer_next(CppLexer* lexer, MLToken* token, MacroEnviron* env, gpointer tag) {
	for(;;) {
		cpp_lexer_next(lexer, token);
		if( token->type==TK_MACRO ) {
			CppLexer mlexer;

			cpp_lexer_init(&mlexer, token->buf, token->len, token->line);
			cpp_parse_macro(env, &mlexer, tag);
			cpp_lexer_final(&mlexer);
			continue;
		}

		if( token->type==TK_ID ) {
			MLStr name = { token->buf, token->len };
			RMacro* macro = (*(env->find_macro))(&name, tag);

			if( !macro )
				break;

			if( macro_replace(env, macro, lexer, token) )
				continue;
		}

		break;
	}
}

