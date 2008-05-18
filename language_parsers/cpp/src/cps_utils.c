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

#define return_if_error(cond, retval) \
	if( cond ) { \
		g_printerr( "ParseError(%s:%d)\n" \
					"	Function : \n" \
					"	Reason   : \n" \
					, __FILE__ \
					, __LINE__ \
					, __FUNCTION__ \
					, #cond ); \
		return retval; \
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

TinyStr* parse_ns(Block* block) {
	gchar buf[MAX_RESULT_LENGTH];
	gchar* p = buf;
	gchar* end = buf + MAX_RESULT_LENGTH;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	return_null_if_error( ps==pe );

	if( ps->type==SG_DBL_COLON ) {
		*p++ = '.';
		++ps;
	}

	for(;;) {
		return_if_error( (ps==pe), 0 );

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

	return tiny_str_new(buf, (p - buf));
}

