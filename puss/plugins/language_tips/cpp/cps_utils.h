// cps_utils.h
// 
#ifndef PUSS_CPP_CPS_UTILS_H
#define PUSS_CPP_CPS_UTILS_H

#include "cps.h"

#ifdef _DEBUG
	#define err_trace(reason) \
		g_printerr( "ParseError(%s:%d)\n" \
				"	Function : %s\n" \
				"	Reason   : %s\n" \
				, __FILE__ \
				, __LINE__ \
				, __FUNCTION__ \
				, reason )
#else
	#define err_trace(reason)
#endif

#define err_return(reason, retval) \
	err_trace(reason); \
	return retval

#define err_return_if(cond, retval) \
	if( cond ) { \
		err_return(#cond, retval); \
	}

#define err_return_if_not(cond, retval)  err_return_if(!(cond), retval)

#define err_return_null_if(cond)         err_return_if(cond, 0)
#define err_return_null_if_not(cond)     err_return_null_if(!(cond))
#define err_return_false_if(cond)        err_return_if(cond, FALSE)
#define err_return_false_if_not(cond)    err_return_false_if(!(cond))

#define err_goto(reason, label) \
	err_trace(reason); \
	goto label

#define err_goto_if(cond, label) \
	if( cond ) { \
		err_goto(#cond, label); \
	}

#define err_goto_if_not(cond, label)   err_goto_if(!(cond), label)

#define err_goto_finish_if(cond)       err_goto_if(cond, __cps_finish__)
#define err_goto_finish_if_not(cond)   err_goto_finish_if(!(cond))

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

TinyStr* block_meger_tokens(MLToken* ps, MLToken* pe, TinyStr* init);

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
MLToken* parse_ns(MLToken* ps, MLToken* pe, TinyStr** ns, MLToken** name_token);
MLToken* parse_datatype(MLToken* ps, MLToken* pe, TinyStr** ns, gint* dt);
MLToken* parse_ptr_ref(MLToken* ps, MLToken* pe, gint* dt);
MLToken* parse_id(MLToken* ps, MLToken* pe, TinyStr** ns, MLToken** name_token);
MLToken* parse_value(MLToken* ps, MLToken* pe);

#endif//PUSS_CPP_CPS_UTILS_H

