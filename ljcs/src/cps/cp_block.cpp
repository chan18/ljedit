// cp_block.cpp
// 

#include "utils.h"

void parse_block(BlockLexer& lexer, Scope& scope) {
	while( lexer.token().type != '{' )
		lexer.next();

	lexer.next();
	parse_scope(lexer, scope);
}

void parse_extern_scope(BlockLexer& lexer, Scope& scope) {
	parse_block(lexer, scope);
}

void parse_impl_block(BlockLexer& lexer, Scope& scope) {
	while( lexer.token().type != '{' )
		lexer.next();

	lexer.next();
	parse_impl_scope(lexer, scope);
}

