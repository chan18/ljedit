// searcher.c
// 

#include "searcher.h"

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

gboolean do_find_key(GString* out, SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith) {
	gchar ch;
	gboolean loop_sign;
	gboolean no_word;

	if( !find_startswith )
		g_string_append_c(out, '$');

	ch = iter_prev(env, ps);
	switch( ch ) {
	case '\0':
		return FALSE;
	case '.':
		if( iter_prev_char(env, ps)=='.' )
			return FALSE;
		break;
	case '>':
		if( iter_prev(env, ps)!='-' )
			return FALSE;
		g_string_append_c(out, ch);
		ch = '-';
		break;
	case '(':
	case '<':
		break;
	case ':':
		if( iter_prev(env, ps)!=':' )
			return FALSE;
		g_string_append_c(out, ch);
		break;
	default:
		if( ch <= 0 )
			return FALSE;

		if( ch!='_' && !g_ascii_isalnum(ch) )
			return FALSE;
	}
	g_string_append_c(out, ch);

	loop_sign = TRUE;
	no_word = TRUE;
	while( loop_sign && ((ch=iter_prev(env, ps)) != '\0') ) {
		switch( ch ) {
		case '.':
			if( no_word )
				return FALSE;
			no_word = TRUE;
			g_string_append_c(out, ch);
			break;

		case ':':
			if( no_word )
				return FALSE;
			no_word = TRUE;

			if( iter_prev_char(env, ps)==':' ) {
				iter_prev(env, ps);
				g_string_append_c(out, ch);
				g_string_append_c(out, ch);
			} else {
				loop_sign = FALSE;
			}
			break;

		case ']':
			iter_skip_pair(env, ps, '[', ']');
			g_string_append_c(out, ']');
			g_string_append_c(out, '[');
			break;
			
		case ')':
			iter_skip_pair(env, ps, '(', ')');
			g_string_append_c(out, ')');
			g_string_append_c(out, '(');
			break;
			
		case '>':
			if( no_word ) {
				if( iter_prev_char(env, ps) != '>' ) {
					iter_skip_pair(env, ps, '<', '>');
					g_string_append_c(out, '>');
					g_string_append_c(out, '<');
				}
				else {
					loop_sign = FALSE;
				}

			} else {
				if( iter_prev_char(env, ps)=='-' ) {
					iter_prev(env, ps);
					g_string_append_c(out, '>');
					g_string_append_c(out, '-');
					
				} else {
					loop_sign = FALSE;
				}
			}
			break;
			
		default:
			if( ch>0 && (g_ascii_isalnum(ch) || ch=='_') ) {
				g_string_append_c(out, 'ch');
				no_word = FALSE;
			} else {
				loop_sign = FALSE;
			}
		}
	}

	iter_next(env, ps);
	return TRUE;
}

gchar* cpp_find_key(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith) {
	GString* out;
	gchar* retval = 0;

	out = g_string_sized_new(4096);

	if( do_find_key(out, env, ps, pe, find_startswith) )
		retval = g_strreverse(out->str);

	g_string_free(out, !retval);

	return retval;
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

typedef struct {
	GHashTable* files;
} CppSTree;

void cpp_stree_do_insert(CppSTree* self, CppFile* file) {
	// TODO : insert elems
	// 

	g_hash_table_insert(self->files, file->filename, cpp_file_ref(file));
}

void cpp_stree_do_remove(CppSTree* self, CppFile* file) {
	// TODO : remove elems
	// 

	g_hash_table_remove(self->files, file->filename);
}

void cpp_stree_insert(CppSTree* self, CppFile* file) {
	CppFile* result = g_hash_table_lookup(self->files, file->filename);
	if( result==file )
		return;

	if( result )
		cpp_stree_do_remove(self, result);

	cpp_stree_do_insert(self, file);
}

void cpp_stree_remove(CppSTree* self, CppFile* file) {
	CppFile* result = g_hash_table_lookup(self->files, file->filename);
	if( result==file )
		cpp_stree_do_remove(self, file);
}

