// cp_var.cpp
// 

#include "utils.h"

void do_parse_var(BlockLexer& lexer, Scope& scope) {
	if( lexer.token().type==(';') )
		return;

	std::string ns;
	int dt = KD_UNK;
	parse_datatype(lexer, ns, dt);

	std::string dtdecl;
	meger_tokens(lexer, lexer.begin(), lexer.pos(), dtdecl);

	for(;;) {
		size_t ps = lexer.pos();
		int prdt = dt;
		parse_ptr_ref(lexer, prdt);

		std::string name;
		parse_id(lexer, name);

		Var* p;
		size_t pos = name.find_last_of('.');
		if( pos==name.npos ) {
			p = create_element<Var>(lexer, name);
		} else {
			p = create_element<Var>(lexer, name.substr(pos));
			name.erase(pos);
			p->nskey = name;
		}
		p->decl = dtdecl;
		p->typekey = ns;
		meger_tokens(lexer, ps, lexer.pos(), p->decl);

		scope_insert(scope, p);

		// int a:32;
		if( lexer.token().type==':' )
			break;

		// int a[]
		if( lexer.token().type=='[' ) {
			lexer.next();
			skip_pair_ccc(lexer);
		}

		// int a = xxx;
		if( lexer.token().type=='=' ) {
			lexer.next();
			parse_value(lexer);
		}

		// int a, b = 5, c;
		if( lexer.token().type==',' ) {
			lexer.next();
			continue;
		} else {
			break;
		}
	}
}

void parse_var(BlockLexer& lexer, Scope& scope) {
	try {
		do_parse_var(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_var!");
		throw e;
	}
}

