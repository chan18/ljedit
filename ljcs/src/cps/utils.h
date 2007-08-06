// utils.h
// 

#ifndef LJCS_PARSERS_UTILS_H
#define LJCS_PARSERS_UTILS_H

#include "blocklexer.h"
#include "../ds_utils.h"

using namespace cpp;

enum Kind { KD_CLASS = KW_CLASS
	, KD_STRUCT = KW_STRUCT
	, KD_UNION = KW_UNION
	, KD_ENUM = KW_ENUM
	, KD_USR
	, KD_STD
	, KD_PTR
	, KD_REF
	, KD_UNK };

void meger_tokens(BlockLexer& lexer, size_t ps, size_t pe, std::string& decl);

// parse ns
// i.e
//  T
//      => [T]
//  ::T
//      => [::, T]
//  std::list<int>::iterator
//      => [std, list, iterator]
//
void parse_ns(BlockLexer& lexer, std::string& ns);

void parse_datatype(BlockLexer& lexer, std::string& ns, int& dt);

void parse_ptr_ref(BlockLexer& lexer, int& dt);

void parse_id(BlockLexer& lexer, std::string& ns);

void parse_value(BlockLexer& lexer);

void parse_scope(BlockLexer& lexer, Scope& scope);

void parse_skip_block(BlockLexer& lexer, Scope& scope);

void parse_impl_scope(BlockLexer& lexer, Scope& scope);

inline void skip_pair_aaa(BlockLexer& lexer) {
	int layer = 1;
	while( layer > 0 ) {
		switch( lexer.token().type ) {
		case '(':	++layer;	break;
		case ')':	--layer;	break;
		}
		lexer.next();
	}
}

inline void skip_pair_bbb(BlockLexer& lexer) {
 	int layer = 1;
 	while( layer > 0 ) {
 		switch( lexer.token().type ) {
		case '<':	++layer;	break;
		case '>':	--layer;	break;
		case '(':
			lexer.next();
			skip_pair_aaa(lexer);
			continue;
		}
		lexer.next();
	}
}

inline void skip_pair_ccc(BlockLexer& lexer) {
 	int layer = 1;
 	while( layer > 0 ) {
 		switch( lexer.token().type ) {
		case '[':	++layer;	break;
		case ']':	--layer;	break;
		case '(':
			lexer.next();
			skip_pair_aaa(lexer);
			continue;
		}
		lexer.next();
	}
}

inline void skip_pair_ddd(BlockLexer& lexer) {
	int layer = 1;
	while( layer > 0 ) {
		switch( lexer.token().type ) {
		case '(':	++layer;	break;
		case ')':	--layer;	break;
		}
		lexer.next();
	}
}
inline void skip_to(BlockLexer& lexer, int type) {
	while( lexer.token().type != type )
		lexer.next();
}

#endif//LJCS_PARSERS_UTILS_H

