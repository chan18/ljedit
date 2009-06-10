// cps.h
// 
#ifndef PUSS_CPP_CPS_H
#define PUSS_CPP_CPS_H

#include "macro_lexer.h"

#define BLOCK_STYLE_LINE	0
#define BLOCK_STYLE_BLOCK	1

#define BLOCK_SCOPE_PUBLIC		0
#define BLOCK_SCOPE_PROTECTED	1
#define BLOCK_SCOPE_PRIVATE		2

#define SPLITER_POLICY_USE_LEXER	0
#define SPLITER_POLICY_USE_TOKENS	1

typedef struct {
	gint			policy;
	ParseEnv*	env
	MLToken*		tokens;
	gint			cap;
	gint			end;
	gint			pos;
} BlockSpliter;

typedef struct {
	CppElem*		parent;
	MLToken*		tokens;
	gsize			count;
	gint			style;
	gint			scope;
} Block;

typedef gboolean (*TParseFn)(ParseEnv* env, Block* block);

void spliter_init(BlockSpliter* spliter, ParseEnv* env);
void spliter_init_with_tokens(BlockSpliter* spliter, ParseEnv* env, MLToken* tokens, gint count);
void spliter_final(BlockSpliter* spliter);
TParseFn spliter_next_block(BlockSpliter* spliter, Block* block);

MLToken* parse_scope(ParseEnv* env, MLToken* tokens, gsize count, CppElem* parent, gboolean use_block_end);
void parse_impl_scope(Block* block);

#endif//PUSS_CPP_CPS_H

