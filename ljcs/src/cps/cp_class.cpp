// cp_class.cpp
// 

#include "utils.h"
#include <sstream>

void parse_class(BlockLexer& lexer, Scope& scope) {
	int clstype = KD_UNK;
	for(;;) {
		Token& tkn = lexer.token();
		lexer.next();
		if( tkn.type==KW_STRUCT || tkn.type==KW_CLASS || tkn.type==KW_UNION ) {
			clstype = tkn.type;
			break;
		}
	}

	// class xxx_export CTest {};
	// 
	// fix : not find xxx_export some times
	// 
	while( lexer.token().type==TK_ID && lexer.token_next().type==TK_ID )
		lexer.next();

	std::string name;
	if( lexer.token().type==TK_ID ) {
		parse_id(lexer, name);

	} else {
		//std::stringstream ss;
		//ss << "@anonymous_" << lexer.block().filename() << ':' << lexer.block().start_line() << '_';
		//name = ss.str();
		name = "@anonymous";
	}

	Class* p;
	size_t pos = name.find_last_of('.');
	if( pos==name.npos ) {
		p = create_element<Class>(lexer, name);
	} else {
		p = create_element<Class>(lexer, name.substr(pos + 1));
		name.erase(pos);
		p->nskey = name;
	}

	p->clstype = clstype==KW_STRUCT ? 's' : (clstype==KW_CLASS ? 'c' : (clstype==KW_UNION ? 'u' : '?'));
	meger_tokens(lexer, lexer.begin(), lexer.pos(), p->decl);
	scope_insert(scope, p);

	if( lexer.token().type==':' ) {
		// parse inheritance
		do {
			if( lexer.next().type==KW_VIRTUAL )
				lexer.next();

			switch( lexer.token().type ) {
			case KW_PUBLIC:
			case KW_PROTECTED:
			case KW_PRIVATE:
				lexer.next();
			}

			std::string inher;
			parse_id(lexer, inher);
			p->inhers.push_back(inher);
		} while( lexer.token().type==',' );
	}

	if( lexer.token().type==';' )
		return;

	throw_parse_error_if(lexer.token().type != '{');
	lexer.next();

	parse_scope(lexer, p->scope, p);

	throw_parse_error_if( lexer.token().type!='}' );
	lexer.next();

	const std::string& dtdecl = name;
	int dt = clstype;

	while( lexer.token().type != ';' ) {
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
			p->nskey.swap(name);
		}
		p->typekey = dtdecl;
		meger_tokens(lexer, ps, lexer.pos(), p->decl);

		scope_insert(scope, p);

		// int a[]
		if( lexer.token().type=='[' )
			skip_pair_ccc(lexer);

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

