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
MLToken* parse_ns(MLToken* ps, MLToken* pe, TinyStr** out);
MLToken* parse_datatype(MLToken* ps, MLToken* pe, TinyStr** out, gint* dt);
MLToken* parse_ptr_ref(MLToken* ps, MLToken* pe, gint* dt);
MLToken* parse_id(MLToken* ps, MLToken* pe, TinyStr** out);
MLToken* parse_value(MLToken* ps, MLToken* pe);

#endif//PUSS_CPP_CPS_UTILS_H

