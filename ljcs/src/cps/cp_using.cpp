// cp_using.cpp
// 

#include "utils.h"

void parse_using(BlockLexer& lexer, Scope& scope) {
	throw_parse_error_if( lexer.token().type!=KW_USING );

	Using* p = create_element<Using>(lexer, "");
	p->isns = ( lexer.next().type==KW_NAMESPACE );
	if( p->isns )
		lexer.next();

	parse_id(lexer, p->nskey);
	throw_parse_error_if( lexer.token_back().type!=TK_ID );
	p->name = lexer.token_back().word;

	meger_tokens(lexer, lexer.begin(), lexer.pos(), p->decl);
	if( p->decl.size() > 5 && p->decl[5]==':' )
		p->decl.insert(5, " ");

	scope_insert(scope, p);
}

