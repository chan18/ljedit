// cps.h
// 
#ifndef PUSS_CPP_CPS_H
#define PUSS_CPP_CPS_H

#include "parser.h"

#define BLOCK_STYLE_LINE	0
#define BLOCK_STYLE_BLOCK	1

#define BLOCK_SCOPE_PUBLIC		0
#define BLOCK_SCOPE_PROTECTED	1
#define BLOCK_SCOPE_PRIVATE		2

#define SPLITER_POLICY_USE_TEXT		0
#define SPLITER_POLICY_USE_TOKENS	1

typedef struct {
	gint		policy;
	CppLexer*	lexer;
	MLToken*	tokens;
	gint		cap;
	gint		end;
	gint		pos;
} BlockSpliter;

typedef struct {
	CppParser*	env;
	MLToken*	tokens;
	gsize		count;
	gint		style;
	gint		scope;
} Block;

typedef gboolean (*TParseFn)(Block* block, CppElem* parent);

void spliter_init_with_text(BlockSpliter* spliter, gchar* buf, gsize len, gint start_line);
void spliter_init_with_tokens(BlockSpliter* spliter, MLToken* tokens, gint count);
void spliter_final(BlockSpliter* spliter);
TParseFn spliter_next_block(BlockSpliter* spliter, Block* block);

#endif//PUSS_CPP_CPS_H

