// macro_lexer.h
// 
#ifndef PUSS_CPP_MACRO_LEXER_H
#define PUSS_CPP_MACRO_LEXER_H

#include "lexer.h"
#include "parser.h"

typedef struct {
	CppParser*			parser;
	GHashTable*			rmacros_table;
	GHashTable*			used_files;
	CppLexer*			lexer;
	CppFile*			file;

	CppIncludePaths*	include_paths;
} ParseEnv;

CppFile* parse_include_file(ParseEnv* env, MLStr* filename, gboolean is_system_header);

void cpp_macro_lexer_init(ParseEnv* env);
void cpp_macro_lexer_final(ParseEnv* env);

void cpp_macro_lexer_next(ParseEnv* env, MLToken* token);

#endif//PUSS_CPP_MACRO_LEXER_H

