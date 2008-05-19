// cps_utils.c
// 

#include "cps_utils.h"


//inline bool is_id_char(char ch) { return ch > 0 && (isalnum(ch) || ch=='_'); }

#define MAX_RESULT_LENGTH (16*1024)

TinyStr* block_meger_tokens(Block* block) {
	gchar buf[MAX_RESULT_LENGTH];
	gchar* p = buf;
	gchar* end = buf + MAX_RESULT_LENGTH;

	gboolean need_space;
	gchar first;
	gchar last;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	p = buf;
	last = '\0';
	for( ; ps < pe; ++ps ) {
		need_space = FALSE;
		first = ps->buf[0];

		if( last=='<' && first=='<' )
			need_space = TRUE;
		else if( last=='>' && first!=':' )
			need_space = TRUE;
		else if( (last=='*' || last=='&') && (first=='_' || g_ascii_isalnum(first)) )	//!='*' && first!='&' && first!=',' && first!=')') )
			need_space = TRUE;
		else if( last==',' && ((first=='_' || g_ascii_isalnum(first)) || first=='.') )
			need_space = TRUE;
		else if( (last=='_' || g_ascii_isalnum(last)) && (first=='_' || g_ascii_isalnum(first)) )
			need_space = TRUE;

		if( need_space ) {
			if( p < end )
				break;
			*p++ = ' ';
		}

		if( ps->len < (gsize)(end - p) )
			break;
		memcpy(p, ps->buf, ps->len);
		p += ps->len;
		last = ps->buf[ps->len - 1];
	}

	return tiny_str_new(buf, (p - buf));
}

#define trace_error(reason) \
	g_printerr( "ParseError(%s:%d)\n" \
				"	Function : %s\n" \
				"	Reason   : %s\n" \
				, __FILE__ \
				, __LINE__ \
				, __FUNCTION__ \
				, reason ) \

#define return_error(reason, retval) \
	trace_error(reason); \
	return retval \

#define return_if_error(cond, retval) \
	if( cond ) { \
		return_error(#cond, retval); \
	} \

#define return_null_if_error(cond) return_if_error(cond, 0)

MLToken* skip_pair_round_brackets(MLToken* ps, MLToken* pe) {
	gint layer = 1;
	while( layer ) {
		if( ps >= pe )
			return 0;

		switch( ps->type ) {
		case '(':	++layer;	break;
		case ')':	--layer;	break;
		}

		++ps;
	}

	return ps;
}

MLToken* skip_pair_angle_bracket(MLToken* ps, MLToken* pe) {
	gint layer = 1;
	while( layer ) {
		if( ps >= pe )
			return 0;

		switch( ps->type ) {
		case '<':	++layer;	break;
		case '>':	--layer;	break;
		case '(':
			ps = skip_pair_round_brackets(ps+1, pe);
			continue;
		}

		++ps;
	}

	return ps;
}

MLToken* skip_pair_square_bracket(MLToken* ps, MLToken* pe) {
 	gint layer = 1;
	while( layer ) {
		if( ps >= pe )
			return 0;

		switch( ps->type ) {
		case '[':	++layer;	break;
		case ']':	--layer;	break;
		case '(':
			ps = skip_pair_round_brackets(ps+1, pe);
			continue;
		}

		++ps;
	}

	return ps;
}

MLToken* skip_pair_brace_bracket(MLToken* ps, MLToken* pe) {
 	gint layer = 1;
	while( layer ) {
		if( ps >= pe )
			return 0;

		switch( ps->type ) {
		case '{':	++layer;	break;
		case '}':	--layer;	break;
		}

		++ps;
	}

	return ps;
}

MLToken* parse_ns(MLToken* ps, MLToken* pe, TinyStr** out) {
	gchar buf[MAX_RESULT_LENGTH];
	gchar* p = buf;
	gchar* end = buf + MAX_RESULT_LENGTH;

	return_null_if_error( ps==pe );
	if( ps->type==SG_DBL_COLON ) {
		*p++ = '.';
		++ps;
	}

	for(;;) {
		return_null_if_error( ps==pe );

		if( ps->type=='~' ) {
			++ps;
			return_null_if_error( ps==pe || ps->type!=TK_ID );
			return_null_if_error( (p + (ps-1)->len + ps->len) >= end);
			memcpy(p, (ps-1)->buf, (ps-1)->len);
			memcpy(p, ps->buf, ps->len);
			p += ((ps-1)->len + ps->len);

		} else if( ps->type==KW_OPERATOR ) {
			return_null_if_error( (p + ps->len) >= end );
			memcpy(p, ps->buf, ps->len);
			p += ps->len;

			++ps;
			return_null_if_error( ps==pe );
			if( ps->type=='(' ) {
				++ps;
				return_null_if_error( ps==pe || ps->type!=')' );
				return_null_if_error( (p + 2) >= end );
				p[0] = '(';
				p[1] = ')';
				p += 2;

			} else if( ps->type=='[' ) {
				++ps;
				return_null_if_error( ps==pe || ps->type!=']' );
				return_null_if_error( (p + 2) >= end );
				p[0] = '[';
				p[1] = ']';
				p += 2;

			} else {
				if( ps->len > 0 && g_ascii_isalpha(ps->buf[0]) && p < end ) {
					*p++ = ' ';
				}

				return_null_if_error( (p + ps->len) >= end );
				memcpy(p, ps->buf, ps->len);
				p += ps->len;
			}

			if( (ps->type==KW_NEW || ps->type==KW_DELETE) && ((ps+1)<pe && (ps+1)->type=='[') ) {
				++ps;
				return_null_if_error( (p + 2) >= end );
				p[0] = '[';
				p[1] = ']';
				p += 2;
			}

		} else {
			if( ps->type==KW_TEMPLATE ) {
				++ps;
				return_null_if_error( ps==pe );
			}


			return_null_if_error( ps->type!=TK_ID );
			return_null_if_error( (p + ps->len) >= end );
			memcpy(p, ps->buf, ps->len);
			p += ps->len;
		}

		++ps;
		return_null_if_error( ps==pe );
		if( ps->type=='<' ) {
			ps = skip_pair_angle_bracket(ps+1, pe);
			return_null_if_error( !ps );
		}

		if( ps->type==SG_DBL_COLON ) {
			++ps;
			return_null_if_error( ps==pe );
			return_null_if_error( p >= end );
			*p++ = '.';
			continue;
		}

		break;
	}

	*out = tiny_str_new(buf, (p - buf));
	return ps;
}

MLToken* parse_datatype(MLToken* ps, MLToken* pe, TinyStr** out, gint* dt) {
	gboolean is_std = FALSE;

	*dt = KD_UNK;
	while( ps < pe ) {
		switch( ps->type ) {
		case KW_EXPORT:		case KW_EXTERN:		case KW_STATIC:
		case KW_AUTO:		case KW_REGISTER:	case KW_CONST:
		case KW_VOLATILE:	case KW_MUTABLE:	case KW_RESTRICT:
			break;

		case KW_VOID:		case KW_BOOL:		case KW_CHAR:
		case KW_WCHAR_T:	case KW_DOUBLE:		case KW_SHORT:
		case KW_FLOAT:		case KW_INT:		case KW_LONG:
		case KW_SIGNED:		case KW_UNSIGNED:	case KW__BOOL:
		case KW__COMPLEX:	case KW__IMAGINARY:
			*dt = KD_STD;
			is_std = TRUE;
			break;

		case TK_ID:
			if( ps->len==6 && memcmp(ps->buf, "size_t", 6)==0 ) {
				*dt = KD_STD;
				is_std = TRUE;
				break;
				
			} else if( is_std || *out ) {
				return ps;
			}
			// not use break;

		case SG_DBL_COLON:
			ps = parse_ns(ps, pe, out);
			if( !ps )
				trace_error("parse ns error when parse datatype!");
			return ps;

		case KW_CLASS:		case KW_STRUCT:		case KW_UNION:
		case KW_TYPENAME:	case KW_ENUM:
			if( is_std )
				return ps;

			*dt = ps->type;
			if( *out )
				return ps;

			if( (ps+1)<pe && (ps+1)->type==TK_ID ) {
				ps = parse_ns(ps, pe, out);
				if( !ps )
					trace_error("parse ns error when parse datatype!");
				return ps;
			}
			break;

		default:
			return_null_if_error( ps->type!=KW_TEMPLATE );
			return ps;
		}

		++ps;
	}

	return_error("eof error", 0);
}

MLToken* parse_ptr_ref(MLToken* ps, MLToken* pe, gint* dt) {
	while( ps < pe ) {
		switch( ps->type ) {
		case KW_EXPORT:		case KW_EXTERN:		case KW_STATIC:
		case KW_INLINE:		case KW_AUTO:		case KW_REGISTER:
		case KW_CONST:		case KW_VOLATILE:	case KW_MUTABLE:
		case KW_VIRTUAL:	case KW_RESTRICT:
			break;
		case '*':
			*dt = KD_PTR;
			break;
		case '&':
			*dt = KD_REF;
			break;
		default:
			return ps;
		}

		++ps;
	}

	return_error("eof error", 0);
}

MLToken* parse_id(MLToken* ps, MLToken* pe, TinyStr** out) {
	ps = parse_ns(ps, pe, out);
	if( !ps )
		trace_error("parse ns error when parse id!");
	return ps;
}

MLToken* parse_value(MLToken* ps, MLToken* pe) {
	gint type;
	while( ps && ps < pe ) {
		type = ps->type;
		if( type==',' || type==';' || type==')' || type=='}' )
			break;

		++ps;
		return_null_if_error( ps==pe );

		switch( type ) {
		case '(':	ps = skip_pair_round_brackets(ps, pe);	break;
		case '<':	ps = skip_pair_angle_bracket(ps, pe);	break;
		case '[':	ps = skip_pair_square_bracket(ps, pe);	break;
		case '{':	ps = skip_pair_brace_bracket(ps, pe);	break;
		}
	}

	if( !ps )
		trace_error("Error when parse value!");

	return ps;
}

