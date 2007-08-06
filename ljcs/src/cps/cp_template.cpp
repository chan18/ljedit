// cp_template.cpp
// 

#include "utils.h"

void parse_template_arg_value(BlockLexer& lexer) {
	try {
		for(;;) {
			int type = lexer.token().type;
			if( type==',' || type=='>' )
				break;

			lexer.next();
			switch( type ) {
			case '(':	skip_pair_aaa(lexer);	break;
			case '<':	skip_pair_bbb(lexer);	break;
			case '[':	skip_pair_ccc(lexer);	break;
			}
		}
		
	} catch( ParseError& e) {
		parse_trace("Error when parse template arg value!");
		throw e;
	}
}

// template<...>
// 
void do_parse_template(BlockLexer& lexer, Scope& scope) {
	skip_to(lexer, KW_TEMPLATE );
	throw_parse_error_if( lexer.next().type != '<' );
	lexer.next();

	Template templ;
	for(;;) {
		int type = lexer.token().type;
		if( type=='>' )
			break;

		templ.args.resize( templ.args.size() + 1 );
		Template::Arg& arg = templ.args.back();
		if( type==KW_TYPENAME || type==KW_CLASS ) {
			arg.type = lexer.token().word;
			if(lexer.next().type==TK_ID ) {
				arg.name = lexer.token().word;
				lexer.next();
			}
		} else {
			int dt = KD_UNK;
			parse_datatype(lexer, arg.type, dt);
			if( lexer.token().type==TK_ID )
				parse_id(lexer, arg.name);
		}

		if( lexer.token().type=='=' ) {
			lexer.next();
			parse_template_arg_value(lexer);
		}

		if( lexer.token().type==',' )
			lexer.next();
		else
			break;
	}

	throw_parse_error_if( lexer.token().type != '>' );

	// <not finished>
	// /usr/include/c++/4.0/bits/basic_string.tcc:88
	// Ä£°åÁ¬Ä£°å
	// 
	if( lexer.next().type==KW_TEMPLATE ) {
		do_parse_template(lexer, scope);
		return;
	}

	parse_scope(lexer, scope);
}

void parse_template(BlockLexer& lexer, Scope& scope) {
	try {
		do_parse_template(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_template!");
		throw e;
	}
}

// extern template class std::string;
// 
void parse_extern_template(BlockLexer& lexer, Scope& scope) {
	// ignore
}

