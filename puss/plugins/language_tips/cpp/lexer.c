// lexer.c
// 

#include "lexer.h"

#include <memory.h>
#include <assert.h>

struct _MLStrNode {
	MLStr		str;
	MLStrNode*	next;
};

static void ml_str_merge(MLStr* out, MLStr* array, gint count) {
	gint i;
	gchar* p;
	gsize len = 0;
	for( i=0; i<count; ++i )
		len += array[i].len;

	if( len==0 )
		return;

	//p = g_slice_alloc(out->len + len + 1 );
	p = cpp_malloc(out->len + len + 1);
	if( out->buf ) {
		memcpy(p, out->buf, out->len);
		//g_slice_free1(out->len + 1, out->buf);
		cpp_free(out->buf);
	}
	out->buf = p;
	p += out->len;
	out->len += len;

	for( i=0; i<count; ++i ) {
		memcpy(p, array[i].buf, array[i].len);
		p += array[i].len;
	}

	g_assert( p==(out->buf + out->len) );
	*p = '\0';
}

void cpp_lexer_frame_push(CppLexer* lexer, MLStr* str, gboolean need_copy_str, gboolean is_new_line, gpointer tag) {
	if( (lexer->top + 1) < FRAME_STACK_MAX ) {
		MLStrNode* node;
		CppFrame* frame;

		// OPT : keeps can use hash_table, and key use MLStr!!!
		//       if find str in hash_table, not need create new node
		//

		//node = g_slice_new0(MLStrNode);
		cpp_init_new(node, MLStrNode, 1);
		if( need_copy_str ) {
			//node->str.buf = g_slice_alloc(str->len + 1);
			node->str.buf = cpp_malloc(str->len + 1);
			memcpy(node->str.buf, str->buf, str->len);
			node->str.buf[str->len] = '\0';
		} else {
			node->str.buf = str->buf;
		}
		node->str.len = str->len;
		node->next = lexer->keeps;
		lexer->keeps = node;

		++lexer->top;

		frame = lexer->stack + lexer->top;
		frame->ps = node->str.buf;
		frame->pe = node->str.buf + node->str.len;
		frame->is_new_line = is_new_line;
		frame->tag = tag;

	} else {
		if( !need_copy_str ) {
			//g_slice_free1(str->len + 1, str->buf);
			cpp_free(str->buf);
		}
	}
}

void cpp_lexer_init(CppLexer* lexer, gchar* text, gsize len, gint start_line) {
	g_assert( lexer && text );

	lexer->keeps = 0;
	lexer->line = start_line;
	lexer->top = 0;
	lexer->stack[0].ps = text;
	lexer->stack[0].pe = text + len;
	lexer->stack[0].is_new_line = TRUE;
	lexer->stack[0].tag = 0;
}

void cpp_lexer_final(CppLexer* lexer) {
	MLStrNode* node;
	while( lexer->keeps ) {
		node = lexer->keeps;
		lexer->keeps = node->next;
		//g_slice_free1(node->str.len + 1, node->str.buf);
		cpp_free(node->str.buf);
		//g_slice_free(MLStrNode, node);
		cpp_free(node);
	}

	lexer->top = -1;
	lexer->line = 0;
}

static gint cpp_lexer_meger_lines_and_skip_ws(CppLexer* lexer);
static gint cpp_lexer_next_sign(CppLexer* lexer, CppFrame* frame);

// use for language witch support utf-8 tag
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

#define INC_LINE()	\
	g_assert( frame==lexer->stack ); \
	++lexer->line;

static gint cpp_lexer_meger_lines_and_skip_ws(CppLexer* lexer) {
	gint line = 0;
	CppFrame* frame;
	gint ch;

label_cpp_lexer_start:
	if( lexer->top < 0 )
		return line;

	frame = &(lexer->stack[lexer->top]);
	g_assert( frame );

	// skip ws
	while( FRAME_HAS_NEXT() ) {
		ch = FRAME_GET_CH();
		if( ch=='\r' ) {
			FRAME_NEXT_CH();
			INC_LINE();
			frame->is_new_line = TRUE;

			if( FRAME_HAS_NEXT() ) {
				ch = FRAME_GET_CH();
				if( ch=='\n' )
					FRAME_NEXT_CH();
			}

		} else if( ch=='\n' ) {
			FRAME_NEXT_CH();
			INC_LINE();
			frame->is_new_line = TRUE;

		} else if( g_ascii_isspace(ch) ) {
			FRAME_NEXT_CH();

		} else {
			break;
		}
	}

	if( line==0 )
		line = lexer->line;

	#define MULTILINES_COUNT 256

	// multi-line meger
	if( frame->is_new_line && FRAME_HAS_NEXT() ) {
		gchar* start = frame->ps;
		MLStr mline = { 0, 0 };
		gint i = 0;
		gboolean in_meger = FALSE;
		MLStr lines[MULTILINES_COUNT];

		// if line endswith '\', meger line
		do {
			lines[i].buf = frame->ps;

			while( FRAME_HAS_NEXT() ) {
				ch = FRAME_GET_CH();
				if( ch=='\\' ) {
					FRAME_NEXT_CH();
					if( FRAME_HAS_NEXT() ) {
						ch = FRAME_GET_CH();
						if( ch=='\r' || ch=='\n' ) {
							in_meger = TRUE;
							++lexer->line;
							lines[i].len = (gsize)(frame->ps - lines[i].buf - 1);
							++i;
							if( i==MULTILINES_COUNT ) {
								ml_str_merge(&mline, lines, i);
								i = 0;
							}
							FRAME_NEXT_CH();

							if( ch=='\r' && FRAME_HAS_NEXT() ) {
								ch = FRAME_GET_CH();
								if( ch=='\n' )
									FRAME_NEXT_CH();
							}
							lines[i].buf = frame->ps;
						}
					}

				} else if( ch=='\r' || ch=='\n' ) {
					if( (i > 0) || mline.buf ) {
						g_assert( i < MULTILINES_COUNT );
						lines[i].len = (gsize)(frame->ps - lines[i].buf - 1);
						++i;

					} else {
						frame->ps = start;
					}

					in_meger = FALSE;
					break;

				} else {
					FRAME_NEXT_CH();
				}
			}

			if( !FRAME_HAS_NEXT() ) {
				if( in_meger ) {
					lines[i].len = (gsize)(frame->ps - lines[i].buf);
					++i;
				}

				break;
			}

		} while( in_meger );

		if( i > 0 )
			ml_str_merge(&mline, lines, i);

		if( mline.buf ) {
			// append mline to file.keeps
			cpp_lexer_frame_push(lexer, &mline, FALSE, TRUE, 0);
			goto label_cpp_lexer_start;
		}

		frame->ps = start;
	}

	// check eof
	if( !FRAME_HAS_NEXT() ) {
		line = 0;

		//cpp_lexer_frame_pop(lexer);
		if( lexer->top >= 0 )
			--lexer->top;

		goto label_cpp_lexer_start;
	}

	return line;
}

void cpp_lexer_next(CppLexer* lexer, MLToken* token) {
	CppFrame* frame;
	gint ch;
	gint line = cpp_lexer_meger_lines_and_skip_ws(lexer);
	if( lexer->top < 0 ) {
		token->type = TK_EOF;
		return;
	}

	frame = &(lexer->stack[lexer->top]);
	ch = FRAME_GET_CH();
	assert( !g_ascii_isspace(ch) );

	token->line = line;
	token->buf = frame->ps;

	// parse macro
	if( frame->is_new_line ) {
		if( ch=='#' ) {
			token->type = TK_MACRO;
			token->buf = frame->ps;
			FRAME_NEXT_CH();
			while( FRAME_HAS_NEXT() ) {
				ch = FRAME_GET_CH();
				if( ch=='\r' || ch=='\n' )
					break;
				FRAME_NEXT_CH();
			}
			goto label_cpp_lexer_finish;
		}
	}

	// lex token->type
	if( ch=='_' || g_ascii_isalpha(ch) ) {
		if( ch=='l' || ch=='L' ) {
			if( FRAME_HAS_NEXT() ) {
				FRAME_NEXT_CH();
				ch = FRAME_GET_CH();
				if( ch=='\'' || ch=='"' )
					goto label_cpp_lexer_sign;
				else
					FRAME_PREV_CH();
			}
		}

		while( FRAME_HAS_NEXT() ) {
			FRAME_NEXT_CH();
			ch = FRAME_GET_CH();
			if( !( ch=='_' || g_ascii_isalnum(ch)) )
				break;
		}

		token->type = TK_ID;
		goto label_cpp_lexer_finish;

	} else if( ch=='+' || ch=='-' || g_ascii_isdigit(ch) ) {
		gboolean in_number;
		if( ch=='+' || ch=='-' ) {
			FRAME_NEXT_CH();
			if( FRAME_HAS_NEXT() ) {
				ch = FRAME_GET_CH();
				if( g_ascii_isdigit(ch) ) {
					FRAME_NEXT_CH();

				} else {
					FRAME_PREV_CH();
					goto label_cpp_lexer_sign;
				}

			} else {
				goto label_cpp_lexer_sign;
			}
		}

		in_number = TRUE;
		while( in_number && FRAME_HAS_NEXT() ) {
			// simple implement, because not need full
			// 
			// 123
			// 0123
			// 0.123
			// 0x123	0X123
			// 123u		123U
			// 123u		123U
			// 123l		123L
			// 123i64	123I64
			// 123ui64	123UI64
			// 0.123f	0.123f
			// 0.12e+3	0.12E+3
			// 0.12e-3	0.12E-3
			// 
			ch = FRAME_GET_CH();
			switch(ch) {
			case '0':	case '1':	case '2':	case '3':	case '4':
			case '5':	case '6':	case '7':	case '8':	case '9':
			case '.':	case 'x':	case 'X':	case 'a':	case 'A':
			case 'b':	case 'B':	case 'c':	case 'C':	case 'd':
			case 'D':	case 'f':	case 'F':	case 'l':	case 'L':
			case 'u':	case 'U':	case 'i':	case 'I':
				FRAME_NEXT_CH();
				break;

			case 'e':	case 'E':
				FRAME_NEXT_CH();
				if( FRAME_HAS_NEXT() ) {
					ch = FRAME_GET_CH();
					if( ch=='+' || ch=='-' )
						FRAME_NEXT_CH();
				}
				break;

			default:
				in_number = FALSE;
				break;
			}
		}

		token->type = TK_NUMBER;
		goto label_cpp_lexer_finish;

	} else if( ch=='/' ) {
		if( FRAME_HAS_NEXT() ) {
			FRAME_NEXT_CH();
			ch = FRAME_GET_CH();
			if( ch=='/' ) {
				token->type = TK_LINE_COMMENT;
				FRAME_NEXT_CH();
				while( FRAME_HAS_NEXT() ) {
					ch = FRAME_GET_CH();
					if( ch=='\r' || ch=='\n' )
						break;
					else
						FRAME_NEXT_CH();
				}
				goto label_cpp_lexer_finish;

			} else if( ch=='*' ) {
				token->type = TK_BLOCK_COMMENT;
				FRAME_NEXT_CH();
				while( FRAME_HAS_NEXT() ) {
					ch = FRAME_GET_CH();
					if( ch=='\r' ) {
						FRAME_NEXT_CH();
						INC_LINE();
						if( FRAME_HAS_NEXT() ) {
							ch = FRAME_GET_CH();
							if( ch=='\n' )
								FRAME_NEXT_CH();
						}
						
					} else if( ch=='\n' ) {
						FRAME_NEXT_CH();
						INC_LINE();

					} else if( ch=='*' ) {
						FRAME_NEXT_CH();
						if( FRAME_HAS_NEXT() ) {
							ch = FRAME_GET_CH();
							if( ch=='/' ) {
								FRAME_NEXT_CH();
								break;
							}
						}
						
					} else {
						FRAME_NEXT_CH();
					}
				}
				goto label_cpp_lexer_finish;

			} else {
				FRAME_PREV_CH();
			}
		}
	}

label_cpp_lexer_sign:
	ch = FRAME_GET_CH();
	if( ch < 0x7f ) {
		token->type = cpp_lexer_next_sign(lexer, frame);

	} else {
		token->type = TK_BAD_WORD;

		while( FRAME_HAS_NEXT() ) {
			FRAME_NEXT_CH();
			ch = FRAME_GET_CH();
			if( ch < 0x7f )
				break;
		}
	}

label_cpp_lexer_finish:
	token->len = (gsize)(frame->ps - token->buf);
}

static gint cpp_lexer_next_sign(CppLexer* lexer, CppFrame* frame) {
	gint ch = FRAME_GET_CH();
	gint type = ch; 

	FRAME_NEXT_CH();

	#define LEX_SIGN_1(ch1, tk1)	\
		if( FRAME_HAS_NEXT() ) { \
			ch = FRAME_GET_CH(); \
			if( ch==ch1 ) { \
				FRAME_NEXT_CH(); \
				type = tk1; \
			} \
		}

	#define LEX_SIGN_2(ch1, tk1, ch2, tk2) \
		if( FRAME_HAS_NEXT() ) { \
			ch = FRAME_GET_CH(); \
			if( ch==ch1 ) { \
				FRAME_NEXT_CH(); \
				type = tk1; \
			} else if( ch==ch2 ) { \
				FRAME_NEXT_CH(); \
				type = tk2; \
			} \
		}

	#define LEX_SIGN_3(ch1, tk1, ch2, tk2, ch3, tk3) \
		if( FRAME_HAS_NEXT() ) { \
			ch = FRAME_GET_CH(); \
			if( ch==ch1 ) { \
				FRAME_NEXT_CH(); \
				type = tk1; \
			} else if( ch==ch2 ) { \
				FRAME_NEXT_CH(); \
				type = tk2; \
			} else if( ch==ch3 ) { \
				FRAME_NEXT_CH(); \
				type = tk3; \
			} \
		}

	switch( ch ) {
	case '\0':
		type = TK_EOF;
		break;

	case '#':
		LEX_SIGN_1('#', SG_DBL_SHARP);
		break;

	case ':':
		LEX_SIGN_1(':', SG_DBL_COLON);
		break;

	case '^':
		LEX_SIGN_1('=', SG_XOR_ASSIGN);
		break;

	case '!':
		LEX_SIGN_1('=', SG_NE);
		break;

	case '&':
		LEX_SIGN_2('=', SG_BIT_AND_ASSIGN, '&', SG_AND);
		if( type==SG_AND )
			LEX_SIGN_1('=', SG_AND_ASSIGN);
		break;

	case '|':
		LEX_SIGN_2('=', SG_BIT_OR_ASSIGN, '|', SG_OR);
		if( type==SG_OR )
			LEX_SIGN_1('=', SG_OR_ASSIGN);
		break;

	case '=':
		LEX_SIGN_1('=', SG_EQ);
		break;

	case '%':
		LEX_SIGN_1('=', SG_MOD_ASSIGN);
		break;

	case '*':
		LEX_SIGN_1('=', SG_MUL_ASSIGN);
		break;

	case '/':
		LEX_SIGN_1('=', SG_DIV_ASSIGN);
		break;

	case '+':
		LEX_SIGN_2('=', SG_ADD_ASSIGN, '+', SG_INC);
		break;

	case '-':
		LEX_SIGN_3('=', SG_SUB_ASSIGN, '-', SG_DEC, '>', SG_PTR_TO);
		if( type==SG_PTR_TO )
			LEX_SIGN_1('*', SG_PTR_STAR);
		break;

	case '\'':
		type = TK_CHAR;
		while( FRAME_HAS_NEXT() ) {
			ch = FRAME_GET_CH();
			if( ch=='\'' ) {
				FRAME_NEXT_CH();
				break;
			} else if( ch=='\r' || ch=='\n' ) {
				break;
			} else if( ch=='\\' ) {
				FRAME_NEXT_CH();
				if( FRAME_HAS_NEXT() )
					FRAME_NEXT_CH();
			} else {
				FRAME_NEXT_CH();
			}
		}
		break;

	case '"':
		type = TK_STRING;
		while( FRAME_HAS_NEXT() ) {
			ch = FRAME_GET_CH();
			if( ch=='\"' ) {
				FRAME_NEXT_CH();
				break;
			} else if( ch=='\r' || ch=='\n' ) {
				break;
			} else if( ch=='\\' ) {
				FRAME_NEXT_CH();
				if( FRAME_HAS_NEXT() )
					FRAME_NEXT_CH();
			} else {
				FRAME_NEXT_CH();
			}
		}
		break;

	case '<':
		LEX_SIGN_2('=', SG_LE, '<', SG_LSTREAM);
		if( type==SG_LSTREAM )
			LEX_SIGN_1('=', SG_LSTREAM_ASSIGN);
		break;

	case '>':
		LEX_SIGN_2('=', SG_GE, '>', SG_RSTREAM);
		if( type==SG_RSTREAM )
			LEX_SIGN_1('=', SG_RSTREAM_ASSIGN);
		break;

	case '.':
		LEX_SIGN_2('*', SG_DOT_STAR, '.', SG_ELLIPSIS);
		if( type==SG_ELLIPSIS ) {
			if( FRAME_HAS_NEXT() ) {
				ch = FRAME_GET_CH();
				if( ch=='.' ) {
					FRAME_NEXT_CH();
					break;
				}
			}
			FRAME_PREV_CH();
			type = '.';
		}
		break;

	default:
		break;
	}

	return type;
}

