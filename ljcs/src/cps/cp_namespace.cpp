// cp_namespace.cpp
// 

#include "utils.h"

void do_parse_namespace(BlockLexer& lexer, Scope& scope) {
	throw_parse_error_if( lexer.token().type!=KW_NAMESPACE );
	
	Scope* ns_scope = &scope;	// anonymous namespace
	if( lexer.next().type!='{' ) {
		Token& name = lexer.token();
		throw_parse_error_if( name.type != TK_ID );
		throw_parse_error_if( lexer.next().type != '{' );
		Namespace* p = create_element<Namespace>(lexer, name.word);
		p->decl = "namesapce ";
		p->decl += name.word;
		scope_insert(scope, p);
		ns_scope = &(p->scope);
	}

	lexer.next();
	parse_scope(lexer, *ns_scope);
}

void parse_namespace(BlockLexer& lexer, Scope& scope) {
	try {
		do_parse_namespace(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_namespace!");
		throw e;
	}
}

