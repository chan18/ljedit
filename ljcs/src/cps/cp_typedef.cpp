// cp_typedef.cpp
// 

#include "utils.h"

void do_parse_normal_typedef(BlockLexer& lexer, Scope& scope) {
	std::string ns;
	int dt = KD_UNK;
	parse_datatype(lexer, ns, dt);

	int dtptr = dt;
	parse_ptr_ref(lexer, dtptr);

	if( lexer.token().type==TK_ID ) {
		// typedef A B;
		Typedef* p = create_element<Typedef>(lexer, lexer.token().word);
		p->typekey = ns;

		lexer.next();
		while( lexer.token().type=='[' ) {
			lexer.next();
			skip_pair_ccc(lexer);
		}

		meger_tokens(lexer, lexer.begin(), lexer.pos(), p->decl);

		scope_insert(scope, p);

	} else {
		// typedef (*TFn)(...);
		// typedef (T::*TFn)(...);
		throw_parse_error_if( lexer.token().type!='(' );
	}
}

void do_parse_complex_typedef(BlockLexer& lexer, Scope& scope) {
	// typedef struct { ... } T, *PT;
	Scope tpscope;
	parse_scope(lexer, tpscope);
	throw_parse_error_if( tpscope.elems.size() < 1 );

	Element* elem = tpscope.elems.front();
	throw_parse_error_if( elem->type!=ET_CLASS && elem->type!=ET_ENUM );
	scope_insert(scope, elem);
	tpscope.elems[0] = 0;

	Elements::iterator it = tpscope.elems.begin();
	Elements::iterator end = tpscope.elems.end();
	for( ++it; it != end; ++it ) {
		throw_parse_error_if( (*it)->type != ET_VAR );
		Var& var = *(Var*)(*it);

		Typedef* p = create_element<Typedef>(lexer, var.name);
		p->typekey.swap(var.typekey);
		p->decl = "typedef ";
		p->decl.append(var.decl);

		scope_insert(scope, p);
	}
}

void parse_typedef(BlockLexer& lexer, Scope& scope) {
	try {
		throw_parse_error_if( lexer.token().type!=KW_TYPEDEF );
		lexer.next();

		if( lexer.block().find('{') == lexer.block().end() )
			do_parse_normal_typedef(lexer, scope);
		else
			do_parse_complex_typedef(lexer, scope);

	} catch(ParseError& e) {
		parse_trace("Error when parse_typedef!");
		throw e;
	}
}

