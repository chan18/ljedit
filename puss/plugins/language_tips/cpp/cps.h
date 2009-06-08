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
	CppFile*	file;
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

typedef struct {
	CppParser*	env;
	CppFile*	file;
} BlockTag;

typedef gboolean (*TParseFn)(Block* block, CppElem* parent);

void spliter_init_with_text(BlockSpliter* spliter, CppFile* file, gchar* buf, gsize len, gint start_line);
void spliter_init_with_tokens(BlockSpliter* spliter, CppFile* file, MLToken* tokens, gint count);
void spliter_final(BlockSpliter* spliter);
TParseFn spliter_next_block(BlockSpliter* spliter, Block* block);

MLToken* parse_scope(CppParser*	env, MLToken* tokens, gsize count, CppElem* parent, gboolean use_block_end);
void parse_impl_scope(Block* block, CppElem* parent);

#endif//PUSS_CPP_CPS_H

