// macro_lexer.h
// 
#ifndef PUSS_CPP_MACRO_LEXER_H
#define PUSS_CPP_MACRO_LEXER_H

#include "lexer.h"

typedef struct {
	MLStr		name;
	gint		argc;
	MLStr*		argv;
	MLStr		value;
} RMacro;

typedef RMacro*	(*TFindMacroFn)(MLStr* name, gpointer tag);
typedef void	(*TOnMacroDefineFn)(MLStr* name, gint argc, MLStr* argv, MLStr* value, MLStr* comment, gpointer tag);
typedef void	(*TOnMacroUndefFn)(MLStr* name, gpointer tag);
typedef void	(*TOnMacroIncludeFn)(MLStr* filename, gboolean is_system_header, gpointer tag);

typedef struct {
	TFindMacroFn		find_macro;
	TOnMacroDefineFn	on_macro_define;
	TOnMacroUndefFn		on_macro_undef;
	TOnMacroIncludeFn	on_macro_include;
} MacroEnviron;

void cpp_macro_lexer_next(CppLexer* lexer, MLToken* token, MacroEnviron* env, gpointer tag);

#endif//PUSS_CPP_MACRO_LEXER_H

