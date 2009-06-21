// search_key.c
// 

#include "guide.h"

#define iter_prev(env, it) env->do_prev(it)
#define iter_next(env, it) env->do_next(it)

inline gchar iter_prev_char(SearchIterEnv* env, gpointer it) {
	gchar ch = env->do_prev(it);
	if( ch )
		env->do_next(it);
	return ch;
}

inline gchar iter_next_char(SearchIterEnv* env, gpointer it) {
	gchar ch = env->do_next(it);
	if( ch )
		env->do_prev(it);
	return ch;
}

void iter_skip_pair(SearchIterEnv* env, gpointer it, gchar sch, gchar ech) {
	gchar ch = '\0';
	gint layer = 1;
	while( layer > 0 ) {
		ch = iter_prev(env, it);
		switch( ch ) {
		case '\0':
		case ';':
		case '{':
		case '}':
			return;
		case ':':
			if( iter_prev_char(env, it)!=':' )
				return;
			break;
		default:
			if( ch==ech )
				++layer;
			else if( ch==sch )
				--layer;
			break;
		}	
	}
}

GList* cpp_find_spath(Searcher* searcher, SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith) {
	gchar ch;
	gboolean loop_sign;
	GList* spath = 0;
	gchar tp;
	GString* buf = g_string_sized_new(512);

	ch = iter_prev(env, ps);

	if( find_startswith ) {
		switch( ch ) {
		case '\0':
			goto find_error;

		case '.':
			if( iter_prev_char(env, ps)=='.' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			ch = iter_prev(env, ps);
			break;

		case '>':
			if( iter_prev(env, ps)!='-' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			ch = iter_prev(env, ps);
			break;

		case '(':
		case '<':
			break;

		case ':':
			if( iter_prev(env, ps)!=':' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			ch = iter_prev(env, ps);
			break;

		default:
			while( ch=='_' || g_ascii_isalnum(ch) ) {
				g_string_append_c(buf, ch);
				ch = iter_prev(env, ps);
			}

			if( buf->len==0 )
				goto find_error;

			spath = g_list_prepend(spath, skey_new('S', buf->str, buf->len));
			g_string_assign(buf, "");
		}
	}

	loop_sign = TRUE;
	while( loop_sign && ((ch=iter_prev(env, ps)) != '\0') ) {
		switch( ch ) {
		case '.':
			if( buf->len==0 )
				return FALSE;
			spath = g_list_prepend(spath, skey_new('S', buf->str, buf->len));
			g_string_append_c(buf, ch);
			break;

		case ':':
			if( buf->len==0 )
				return FALSE;

			if( iter_prev_char(env, ps)==':' ) {
				iter_prev(env, ps);
				spath = g_list_prepend(spath, skey_new('?', buf->str, buf->len));
				g_string_append_c(buf, ch);
			} else {
				loop_sign = FALSE;
			}
			break;

		case ']':
			iter_skip_pair(env, ps, '[', ']');
			spath = g_list_prepend(spath, skey_new('v', buf->str, buf->len));
			g_string_append_c(buf, ch);
			break;
			
		case ')':
			iter_skip_pair(env, ps, '(', ')');
			spath = g_list_prepend(spath, skey_new('f', buf->str, buf->len));
			g_string_append_c(buf, ch);
			break;
			
		case '>':
			if( buf->len==0 ) {
				if( iter_prev_char(env, ps) != '>' ) {
					iter_skip_pair(env, ps, '<', '>');
				} else {
					loop_sign = FALSE;
				}

			} else {
				if( iter_prev_char(env, ps)=='-' ) {
					iter_prev(env, ps);
					spath = g_list_prepend(spath, skey_new('v', buf->str, buf->len));
					g_string_append_c(buf, ch);
					
				} else {
					loop_sign = FALSE;
				}
			}
			break;
			
		default:
			if( ch=='_' || g_ascii_isalnum(ch) ) {
				do {
					g_string_append_c(buf, ch);
					ch = iter_prev(env, ps);
				} while( ch=='_' || g_ascii_isalnum(ch) );
			} else {
				loop_sign = FALSE;
			}
		}
	}

	iter_next(env, ps);
	goto find_finish;

find_error:
	g_list_free(spath);
	spath = 0;

find_finish:
	g_string_free(buf, TRUE);
	return spath;
}

typedef struct {
	const gchar* start;
	const gchar* end;
	gchar* cur;
} ParseKeyIter;

static gchar parse_key_do_prev(ParseKeyIter* pos) {
	return (pos->cur > pos->start) ? *(--(pos->cur)) : '\0';
}

static gchar parse_key_do_next(ParseKeyIter* pos) {
	return (pos->cur < pos->end) ? *(++(pos->cur)) : '\0';
}

gchar* cpp_parse_key(const gchar* text, gboolean find_startswith) {
	gint len = strlen(text);
	ParseKeyIter ps = { text, text+len, text+len };
	ParseKeyIter pe = { text, text+len, text+len };
	SearchIterEnv env = { parse_key_do_prev, parse_key_do_next };

	return cpp_find_key(&env, &ps, &pe, find_startswith);
}

