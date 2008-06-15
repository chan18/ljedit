// parser.h
// 
#ifndef PUSS_CPP_PARSER_H
#define PUSS_CPP_PARSER_H

#include "ds.h"
#include "macro_lexer.h"

typedef struct {
	MacroEnviron	macro_environ;
	GHashTable*		rmacros_table;
	gboolean		enable_macro_replace;
	gpointer		keywords_table;

	void			(*pe_file_incref)(CppFile* file);
	void			(*pe_file_decref)(CppFile* file);
} CppParser;

void cpp_parser_init(CppParser* env, gboolean enable_macro_replace);
void cpp_parser_final(CppParser* env);

CppFile* cpp_parser_parse(CppParser* env, gchar* filename_buf, gsize filename_len, gchar* buf, gsize len);

#endif//PUSS_CPP_PARSER_H

