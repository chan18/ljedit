// test.c
// 

#include "parser.h"


void lexer_test(gchar* buf, gsize len) {
	CppLexer lexer;
	MLToken token;
	cpp_lexer_init(&lexer, buf, len, 1);
	do {
		cpp_lexer_next(&lexer, &token);
	} while( token.type!=TK_EOF );
	
	cpp_lexer_final(&lexer);
}

static inline void mlstr_cpy(MLStr* dst, MLStr* src) {
	if( src->buf ) {
		dst->buf = g_slice_alloc(src->len + 1);
		dst->len = src->len;
		memcpy(dst->buf, src->buf, dst->len);
		dst->buf[dst->len] = '\0';
	}
}

static RMacro* rmacro_new(MLStr* name, gint argc, MLStr* argv, MLStr* value) {
	RMacro* macro = g_slice_new0(RMacro);
	mlstr_cpy( &(macro->name), name );

	macro->argc = argc;
	if( argc > 0 ) {
		gint i;
		macro->argv = g_slice_alloc(sizeof(MLStr) * argc);
		for( i=0; i<argc; ++i )
			mlstr_cpy(macro->argv + i, argv + i);
	}

	mlstr_cpy(&(macro->value), value);

	return macro;
}

static void rmacro_free(RMacro* macro) {
	if( macro ) {
		g_slice_free1(macro->name.len + 1, macro->name.buf);
		if( macro->argv ) {
			gint i;
			for( i=0; i<macro->argc; ++i )
				g_slice_free1((macro->argv + i)->len + 1, (macro->argv + i)->buf);
			g_slice_free1(sizeof(MLStr) * macro->argc, macro->argv);
		}
		g_slice_free1(macro->value.len + 1, macro->value.buf);

		g_slice_free(RMacro, macro);
	}
}

static RMacro* ml_find_macro(MLStr* name, GHashTable* rmacros_table) {
	RMacro* res;
	gchar ch = name->buf[name->len];
	name->buf[name->len] = '\0';
	res = g_hash_table_lookup(rmacros_table, name->buf);
	name->buf[name->len] = ch;
	return res;
}

static void ml_on_macro_define(MLStr* name, gint argc, MLStr* argv, MLStr* value, MLStr* comment, GHashTable* rmacros_table) {
	RMacro* node = rmacro_new(name, argc, argv, value);
	g_hash_table_replace(rmacros_table, node->name.buf, node);
}

static void ml_on_macro_undef(MLStr* name, GHashTable* rmacros_table) {
	gchar ch = name->buf[name->len];
	name->buf[name->len] = '\0';
	g_hash_table_remove(rmacros_table, name->buf);
	name->buf[name->len] = ch;
}

static void ml_on_macro_include(MLStr* filename, gboolean is_system_header, GHashTable* rmacros_table) {
}

void macro_lexer_test(gchar* buf, gsize len) {
	CppLexer lexer;
	MLToken token;
	MacroEnviron env;
	GHashTable* rmacros_table;

	rmacros_table = g_hash_table_new_full(g_str_hash, g_str_equal, 0, (GDestroyNotify)rmacro_free);

	env.find_macro = ml_find_macro;
	env.on_macro_define = ml_on_macro_define;
	env.on_macro_undef = ml_on_macro_undef;
	env.on_macro_include = ml_on_macro_include;

	cpp_lexer_init(&lexer, buf, len, 1);
	do {
		cpp_macro_lexer_next(&lexer, &token, &env, rmacros_table);
	} while( token.type!=TK_EOF );
	
	cpp_lexer_final(&lexer);
	g_hash_table_destroy(rmacros_table);
}

void parser_test(gchar* buf, gsize len) {
	CppParser env;
	CppFile* file;

	cpp_parser_init(&env, TRUE);

	file = cpp_parser_parse(&env, "tee.cpp", 7, buf, len);
	cpp_file_clear(file);
	g_free(file);

	cpp_parser_final(&env);
}

int main(int argc, char* argv[]) {
	gchar* buf;
	gsize len;
	GTimer* timer;
	gdouble used;
	gint i;

	g_mem_set_vtable(glib_mem_profiler_table);

	g_file_get_contents("tee.cpp", &buf, &len, 0);
	if( !buf ) {
		g_print("load file failed!\n");
		return 1;
	}

	#define n 100

	timer = g_timer_new();
	for(i=0; i<n; ++i) {
		//lexer_test(buf, len);
		//macro_lexer_test(buf, len);
		parser_test(buf, len);
	}
	used = g_timer_elapsed(timer, NULL);
	g_timer_destroy(timer);

	g_free(buf);

	g_mem_profile();
	g_print("%f - %f\n", used, used/n);
	return 0;
}

