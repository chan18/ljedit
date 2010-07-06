// lexer.h
// 
#ifndef PUSS_CPP_LEXER_H
#define PUSS_CPP_LEXER_H

#include "token.h"

typedef struct {
	gchar*	buf;
	gsize	len;
} MLStr;

typedef struct _CppFrame	CppFrame;

struct _CppFrame {
	gchar*		ps;
	gchar*		pe;
	gboolean	is_new_line;
	gpointer	tag;
};

#define FRAME_STACK_MAX 32

typedef struct _MLStrNode	MLStrNode;
typedef struct _CppLexer	CppLexer;

struct _CppLexer {
	MLStrNode*		keeps;
	gint			line;
	gint			top;
	CppFrame		stack[FRAME_STACK_MAX];
};

void cpp_lexer_frame_push(CppLexer* lexer, MLStr* str, gboolean need_copy_str, gboolean is_new_line, gpointer tag);

void cpp_lexer_init(CppLexer* lexer, gchar* text, gsize len, gint start_line);
void cpp_lexer_final(CppLexer* lexer);

void cpp_lexer_next(CppLexer* lexer, MLToken* token);

#endif//PUSS_CPP_LEXER_H

