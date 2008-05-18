// cps_utils.h
// 
#ifndef PUSS_CPP_CPS_UTILS_H
#define PUSS_CPP_CPS_UTILS_H

#include "cps.h"

enum Kind { KD_CLASS = KW_CLASS
	, KD_STRUCT = KW_STRUCT
	, KD_UNION = KW_UNION
	, KD_ENUM = KW_ENUM
	, KD_USR
	, KD_STD
	, KD_PTR
	, KD_REF
	, KD_UNK
};

TinyStr* block_meger_tokens(Block* block);

MLToken* skip_pair_round_brackets(MLToken* ps, MLToken* pe);
MLToken* skip_pair_angle_bracket(MLToken* ps, MLToken* pe);
MLToken* skip_pair_square_bracket(MLToken* ps, MLToken* pe);
MLToken* skip_pair_brace_bracket(MLToken* ps, MLToken* pe);

// parse ns
// i.e
//  T
//      => [T]
//  ::T
//      => [::, T]
//  std::list<int>::iterator
//      => [std, list, iterator]
//
TinyStr* parse_ns(Block* block);

/*
void parse_datatype(BlockLexer& lexer, std::string& ns, int& dt);

void parse_ptr_ref(BlockLexer& lexer, int& dt);

void parse_id(BlockLexer& lexer, std::string& ns);

void parse_value(BlockLexer& lexer);

void parse_scope(BlockLexer& lexer, Scope& scope, Element* parent=0);

void parse_skip_block(BlockLexer& lexer, Scope& scope);

void parse_impl_scope(BlockLexer& lexer, Scope& scope);

*/

#endif//PUSS_CPP_CPS_UTILS_H

