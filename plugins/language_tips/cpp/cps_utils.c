// cps_utils.c
// 

#include "cps_utils.h"

#define MAX_RESULT_LENGTH (16*1024)

TinyStr* block_meger_tokens(MLToken* ps, MLToken* pe, TinyStr* init) {
	gchar buf[MAX_RESULT_LENGTH];
	gchar* p = buf;
	gchar* end = buf + MAX_RESULT_LENGTH;

	gboolean need_space = FALSE;
	gchar first = '\0';
	gchar last  = init ? init->buf[tiny_str_len(init)-1] : '\0';

	if( init ) {
		if( tiny_str_len(init) >= MAX_RESULT_LENGTH )
			return 0;

		memcpy(buf, init->buf, tiny_str_len(init));
		p += tiny_str_len(init);
		last = init->buf[tiny_str_len(init) - 1];
	} else {
		last = '\0';
	}

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
		else if( (last=='_' || g_ascii_isalnum(last)) && (first=='~' || first=='_' || g_ascii_isalnum(first)) )
			need_space = TRUE;

		if( need_space ) {
			if( p >= end )
				return 0;
			*p++ = ' ';
		}

		if( (p + ps->len) >= end )
			return 0;
		memcpy(p, ps->buf, ps->len);
		p += ps->len;

		if( ps->type==KW_USING ) {
			if( p >= end )
				return 0;
			*p++ = ' ';
		}

		last = *(p-1);
	}

	if( (p - buf) > 0xffff || (p-buf) < 0 )
		p = p;

	return tiny_str_new(buf, (p - buf));
}

MLToken* skip_pair_round_brackets(MLToken* ps, MLToken* pe) {
	gint layer = 1;
	while( layer ) {
		err_return_null_if_not( ps < pe );

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
		err_return_null_if_not( ps < pe );

		switch( ps->type ) {
		case '<':	++layer;	break;
		case '>':	--layer;	break;
		case '(':
			err_return_null_if( (ps = skip_pair_round_brackets(ps+1, pe))==0 );
			continue;
		}

		++ps;
	}

	return ps;
}

MLToken* skip_pair_square_bracket(MLToken* ps, MLToken* pe) {
 	gint layer = 1;
	while( layer ) {
		err_return_null_if_not( ps < pe );

		switch( ps->type ) {
		case '[':	++layer;	break;
		case ']':	--layer;	break;
		case '(':
			err_return_null_if( (ps = skip_pair_round_brackets(ps+1, pe))==0 );
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

MLToken* parse_ns(MLToken* ps, MLToken* pe, TinyStr** ns, MLToken** name_token) {
	gchar buf[MAX_RESULT_LENGTH];
	gchar* p = buf;
	gchar* end = buf + MAX_RESULT_LENGTH;

	err_return_null_if_not( ps < pe );
	if( ps->type==SG_DBL_COLON ) {
		*p++ = '.';
		++ps;
	}

	for(;;) {
		err_return_null_if_not( ps < pe );

		if( ps->type=='~' ) {
			++ps;
			err_return_null_if_not( (ps<pe) && ps->type==TK_ID );
			err_return_null_if_not( (p + (ps-1)->len + ps->len) < end );
			memcpy(p, (ps-1)->buf, (ps-1)->len);
			p += (ps-1)->len;
			memcpy(p, ps->buf, ps->len);
			p += ps->len;
			if( name_token )
				*name_token = ps;

		} else if( ps->type==KW_OPERATOR ) {
			if( name_token )
				*name_token = ps;

			err_return_null_if_not( (p + ps->len) < end );
			memcpy(p, ps->buf, ps->len);
			p += ps->len;

			++ps;
			err_return_null_if_not( ps < pe );
			if( ps->type=='(' ) {
				++ps;
				err_return_null_if_not( (ps<pe) && ps->type==')' );
				err_return_null_if_not( (p + 2) < end );
				p[0] = '(';
				p[1] = ')';
				p += 2;

			} else if( ps->type=='[' ) {
				++ps;
				err_return_null_if_not( (ps<pe) && ps->type==']' );
				err_return_null_if_not( (p + 2) < end );
				p[0] = '[';
				p[1] = ']';
				p += 2;

			} else {
				if( ps->len > 0 && g_ascii_isalpha(ps->buf[0]) && p < end )
					*p++ = ' ';

				err_return_null_if_not( (p + ps->len) < end );
				memcpy(p, ps->buf, ps->len);
				p += ps->len;
			}

			if( (ps->type==KW_NEW || ps->type==KW_DELETE) && ((ps+1)<pe && (ps+1)->type=='[') ) {
				++ps;
				err_return_null_if_not( (p + 2) < end );
				p[0] = '[';
				p[1] = ']';
				p += 2;
			}

		} else {
			if( ps->type==KW_TEMPLATE ) {
				++ps;
				err_return_null_if_not( ps < pe );
			}

			err_return_null_if_not( ps->type==TK_ID );
			err_return_null_if_not( (p + ps->len) < end );
			if( name_token )
				*name_token = ps;

			memcpy(p, ps->buf, ps->len);
			p += ps->len;
		}

		++ps;
		err_return_null_if_not( ps < pe );
		if( ps->type=='<' ) {
			ps = skip_pair_angle_bracket(ps+1, pe);
			err_return_null_if_not( ps );
		}

		if( ps->type==SG_DBL_COLON ) {
			++ps;
			err_return_null_if_not( ps < pe );
			err_return_null_if_not( p < end );
			*p++ = '.';
			continue;
		}

		break;
	}

	*ns = tiny_str_new(buf, (p - buf));
	return ps;
}

MLToken* parse_datatype(MLToken* ps, MLToken* pe, TinyStr** ns, gint* dt) {
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
				
			} else if( is_std || *ns ) {
				return ps;
			}
			// not use break;

		case SG_DBL_COLON:
			ps = parse_ns(ps, pe, ns, 0);
			if( !ps )
				err_trace("parse ns error when parse datatype!");
			return ps;

		case KW_CLASS:		case KW_STRUCT:		case KW_UNION:
		case KW_TYPENAME:	case KW_ENUM:
			if( is_std )
				return ps;

			*dt = ps->type;
			if( *ns )
				return ps;

			++ps;
			if( ps<pe && ps->type==TK_ID ) {
				ps = parse_ns(ps, pe, ns, 0);
				if( !ps )
					err_trace("parse ns error when parse datatype!");
				return ps;
			}
			break;

		default:
			err_return_null_if( ps->type==KW_TEMPLATE );
			return ps;
		}

		++ps;
	}

	err_return("eof error", 0);
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

	err_return("eof error", 0);
}

MLToken* parse_id(MLToken* ps, MLToken* pe, TinyStr** ns, MLToken** name_token) {
	ps = parse_ns(ps, pe, ns, name_token);
	if( ps ) {
		if( name_token )
			g_assert( *name_token );
	} else {
		err_trace("parse ns error when parse id!");
	}

	return ps;
}

MLToken* parse_value(MLToken* ps, MLToken* pe) {
	gint type;
	while( ps && ps < pe ) {
		type = ps->type;
		if( type==',' || type==';' || type==')' || type=='}' )
			break;

		++ps;
		err_return_null_if_not( ps < pe );

		switch( type ) {
		case '(':	ps = skip_pair_round_brackets(ps, pe);	break;
		case '<':	ps = skip_pair_angle_bracket(ps, pe);	break;
		case '[':	ps = skip_pair_square_bracket(ps, pe);	break;
		case '{':	ps = skip_pair_brace_bracket(ps, pe);	break;
		}
	}

	if( !ps )
		err_trace("Error when parse value!");

	return ps;
}

