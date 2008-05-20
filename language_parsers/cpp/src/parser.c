// parser.c
// 

#include "parser.h"

#include "keywords.h"
#include "cps.h"


static inline void mlstr_cpy(MLStr* dst, MLStr* src) {
	if( src->buf ) {
		gchar ch = src->buf[src->len];
		src->buf[src->len] = '\0';
		dst->buf = g_strdup(src->buf);
		src->buf[src->len] = ch;
		dst->len = src->len;
	}
}

static RMacro* rmacro_new(MLStr* name, gint argc, MLStr* argv, MLStr* value) {
	RMacro* macro = g_new0(RMacro, 1);
	mlstr_cpy( &(macro->name), name );

	macro->argc = argc;
	if( argc > 0 ) {
		gint i;
		macro->argv = g_new(MLStr, argc);
		for( i=0; i<argc; ++i )
			mlstr_cpy(macro->argv + i, argv + i);
	}

	mlstr_cpy(&(macro->value), value);

	return macro;
}

static void rmacro_free(RMacro* macro) {
	if( macro ) {
		g_free(macro->name.buf);
		if( macro->argv ) {
			gint i;
			for( i=0; i<macro->argc; ++i )
				g_free((macro->argv + i)->buf);
			g_free(macro->argv);
		}
		g_free(macro->value.buf);

		g_free(macro);
	}
}

static RMacro* find_macro(MLStr* name, CppParser* env) {
	RMacro* res;
	gchar ch = name->buf[name->len];
	name->buf[name->len] = '\0';
	res = g_hash_table_lookup(env->rmacros_table, name->buf);
	name->buf[name->len] = ch;
	return res;
}

static void on_macro_define(MLStr* name, gint argc, MLStr* argv, MLStr* value, MLStr* comment, CppParser* env) {
	RMacro* node = rmacro_new(name, argc, argv, value);
	g_hash_table_replace(env->rmacros_table, node->name.buf, node);
}

static void on_macro_undef(MLStr* name, CppParser* env) {
	gchar ch = name->buf[name->len];
	name->buf[name->len] = '\0';
	g_hash_table_remove(env->rmacros_table, name->buf);
	name->buf[name->len] = ch;
}

static void on_macro_include(MLStr* filename, gboolean is_system_header, CppParser* env) {
}

void cpp_parser_init(CppParser* env, gboolean enable_macro_replace) {
	env->rmacros_table = g_hash_table_new_full(g_str_hash, g_str_equal, 0, (GDestroyNotify)rmacro_free);
	env->enable_macro_replace = enable_macro_replace;
	env->keywords_table = cpp_keywords_table_new();
	env->macro_environ.find_macro = (TFindMacroFn)find_macro;
	env->macro_environ.on_macro_define = (TOnMacroDefineFn)on_macro_define;
	env->macro_environ.on_macro_undef = (TOnMacroUndefFn)on_macro_undef;
	env->macro_environ.on_macro_include = (TOnMacroIncludeFn)on_macro_include;
}

void cpp_parser_final(CppParser* env) {
	g_hash_table_destroy(env->rmacros_table);
	cpp_keywords_table_free(env->keywords_table);
}

CppFile* cpp_parser_parse(CppParser* env, gchar* buf, gsize len) {
	return 0;
}


static void __test_lexer(CppParser* env, gchar* buf, gsize len) {
	gchar ch;
	gint count;

	MLToken token;
	CppLexer lexer;

	count = 0;
	cpp_lexer_init(&lexer, buf, len, 1);
	for(;;) {
		++count;
		//cpp_lexer_next(&lexer, &token);
		cpp_macro_lexer_next(&lexer, &token, &(env->macro_environ), env);
		if( token.type==TK_EOF ) {
			//g_print("token number : %d\n", count);
			break;
		} else if( token.type==TK_ID ) {
			cpp_keywords_check(&token, env->keywords_table);
		}

		ch = token.buf[token.len];
		token.buf[token.len] = '\0';
		g_print("%d - %s\n", token.type, token.buf);
		token.buf[token.len] = ch;
	}
	cpp_lexer_final(&lexer);
}

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

static void __test_spliter(CppParser* env, gchar* buf, gsize len) {
	Block block;
	TParseFn fn;
	BlockSpliter spliter;

	spliter_init_with_text(&spliter, env, buf, len, 1);

	while( (fn = spliter_next_block(&spliter, &block)) != 0 ) {
		__dump_block(&block);
		fn(&block, 0);
	}

	spliter_final(&spliter);
}

void cpp_parser_test(CppParser* env, gchar* buf, gsize len) {
	//__test_lexer(env, buf, len);
	__test_spliter(env, buf, len);
}
