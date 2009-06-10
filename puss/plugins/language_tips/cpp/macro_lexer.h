// macro_lexer.h
// 
#ifndef PUSS_CPP_MACRO_LEXER_H
#define PUSS_CPP_MACRO_LEXER_H

#include "lexer.h"
#include "parser.h"

typedef struct {
	MLStr		name;
	gint		argc;
	MLStr*		argv;
	MLStr		value;
} RMacro;

typedef struct {
	CppParser*		parser;
	GHashTable*		rmacros_table;
	CppLexer*		lexer
	CppFile*		file;
} ParseEnv;

void cpp_macro_lexer_next(ParseEnv* env, MLToken* token);

#endif//PUSS_CPP_MACRO_LEXER_H

