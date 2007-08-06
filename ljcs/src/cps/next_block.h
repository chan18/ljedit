// next_block.h
// 

#ifndef LJCS_CPS_NEXT_BLOCK_H
#define LJCS_CPS_NEXT_BLOCK_H

#include "blocklexer.h"

struct BreakOutError {};

class Lexer;

typedef void (*TParseFn)(BlockLexer& lexer, cpp::Scope& scope);

bool next_block(Block& block, TParseFn& fn, Lexer* lexer=0);

bool next_fun_impl_block(Block& block, TParseFn& fn);

#endif//LJCS_CPS_NEXT_BLOCK_H

