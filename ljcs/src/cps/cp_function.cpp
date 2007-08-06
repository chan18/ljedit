// cp_function.cpp
// 

#include "utils.h"

void parse_function_prefix(BlockLexer& lexer) {
	// º¯ÊýÇ°×º
	for(;;) {
		switch( lexer.token().type ) {
		case KW_EXTERN :
			if( lexer.token_next().type==TK_STRING )
				lexer.next();
			lexer.next();
			break;

		case KW_STATIC:
		case KW_INLINE:
		case KW_VIRTUAL:
		case KW_EXPLICIT:
		case KW_FRIEND:
			lexer.next();
			break;

		default:
			return;
		}
	}
}

void parse_function_args(BlockLexer& lexer, Function& fun) {
	bool need_save_args = lexer.block().tag=="{";

	while( lexer.token().type != (')') ) {
		int type = lexer.token().type;
		if( type==SG_ELLIPSIS ) {
			lexer.next();
			break;
		}
		
		size_t start_pos = lexer.pos();
		std::string ns;
		int dt = KD_UNK;
		parse_datatype(lexer, ns, dt);
		parse_ptr_ref(lexer, dt);

		std::string name;
		if( lexer.token().type=='(' ) {
			// try parse function pointer
			lexer.next();
			skip_pair_aaa(lexer);
			throw_parse_error_if( lexer.token().type!='(' );
			lexer.next();
			skip_pair_aaa(lexer);
			while( lexer.token().type!=',' && lexer.token().type!=')' )
				lexer.next();
			continue;
		}

		// try find arg name
		type = lexer.token().type;
		if( type!=',' && type!=')' && type!='=' ) {
			parse_id(lexer, name);
		} else {
			name = "@anonymous_funarg";
		}

		if( need_save_args ) {
			Var* p;
			size_t pos = name.find_last_of('.');
			if( pos==name.npos ) {
				p = create_element<Var>(lexer, name);
			} else {
				p = create_element<Var>(lexer, name.substr(pos));
				name.erase(pos);
				p->nskey = name;
			}
			p->typekey = ns;

			meger_tokens(lexer, start_pos, lexer.pos(), p->decl);
			scope_insert(fun.impl, p);
		}

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

		// void foo(int a, int b = 5);
		type = lexer.token().type;
		if( type==',' ) {
			lexer.next();
			continue;
		} else {
			break;
		}
	}
}

void parse_function_common(BlockLexer& lexer, Scope& scope, std::string& ns, std::string& name) {
	Function* p;
	size_t pos = name.find_last_of('.');
	if( pos==name.npos ) {
		p = create_element<Function>(lexer, name);
	} else {
		p = create_element<Function>(lexer, name.substr(pos + 1));
		name.erase(pos);
		p->nskey.swap(name);
	}
	p->typekey = ns;

	std::auto_ptr<Function> ptr(p);

	throw_parse_error_if( lexer.token().type!='(' );
	lexer.next();

	parse_function_args(lexer, *p);

	throw_parse_error_if( lexer.token().type!=')' );
	lexer.next();

	while( lexer.token().type != ';' && lexer.token().type != ':' && lexer.token().type != '{' )
		lexer.next();

	meger_tokens(lexer, lexer.begin(), lexer.pos(), p->decl);
	scope_insert(scope, ptr.release());

	bool has_impl = false;	
	try {
		if( lexer.token().type==':' ) {
			while( lexer.token().type != '{' )
				lexer.next();
		}

		if( lexer.token().type == '{' )
			has_impl = true;
		
	} catch(ParseError) {
	}

	if( has_impl ) {
		//parse_function_implement
		parse_impl_scope(lexer, p->impl);
	} else {
		p->impl.elems.clear();
	} 
}

void do_parse_function(BlockLexer& lexer, Scope& scope) {
	parse_function_prefix(lexer);

	std::string ns; int dt = KD_UNK;
	parse_datatype(lexer, ns, dt);

	int dtptr = dt;
	parse_ptr_ref(lexer, dtptr);

	std::string name;
	if( lexer.token().type=='(' ) {
		// try parse function pointer
		size_t fptrypos = lexer.pos();
		while( lexer.next().type==TK_ID ) {
			if( lexer.next().type=='<' ) {
				lexer.next();
				skip_pair_bbb(lexer);
			}
			if( lexer.token().type!=SG_DBL_COLON ) {
				lexer.pos(fptrypos);
				break;
			}
		}

		if( lexer.token().type=='*' && lexer.next().type==TK_ID ) {
			// function pointer
			name = lexer.token().word;
			throw_parse_error_if( lexer.next().type!=')' );
			lexer.next();
				
		} else {
			// no return value function
			lexer.pos(fptrypos);

			name = ns;
		}

	} else {
		// normal function
		parse_id(lexer, name);
	}

	parse_function_common(lexer, scope, ns, name);
}

void parse_function(BlockLexer& lexer, Scope& scope) {
	int* p = new int(3);
	try {
		do_parse_function(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_function!");
		throw e;
	}
}

void do_parse_destruct(BlockLexer& lexer, Scope& scope) {
	parse_function_prefix(lexer);

	std::string ns;
	std::string name;
	parse_id(lexer, name);

	parse_function_common(lexer, scope, ns, name);
}

void parse_destruct(BlockLexer& lexer, Scope& scope) {
	try {
		do_parse_destruct(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_destruct!");
		throw e;
	}
}

void parse_operator(BlockLexer& lexer, Scope& scope) {
	try {
		do_parse_function(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_operator_function!");
		throw e;
	}
}

