// cp_class.cpp
// 

#include "utils.h"
#include <sstream>

void parse_enum_iterms(BlockLexer& lexer, Scope& scope) {
	while( lexer.token().type==TK_ID ) {
		Token& tkn = lexer.token();
		EnumItem* p = cpp::Element::create<EnumItem>(lexer.file(), tkn.word, tkn.line, tkn.line);
		p->decl = tkn.word;

		scope_insert(scope, p);

		if( lexer.next().type=='=' ) {
			lexer.next();
			parse_value(lexer);
		}

		if( lexer.token().type==',' )
			lexer.next();
	}
}

void do_parse_enum(BlockLexer& lexer, Scope& scope) {
	skip_to(lexer, KW_ENUM);
	lexer.next();

	std::string name;
	if( lexer.token().type==TK_ID ) {
		name = lexer.token().word;
		lexer.next();
	} else {
		std::stringstream ss;
		ss << "@anonymous_" << lexer.block().filename() << ':' << lexer.block().start_line() << '_';
		name = ss.str();
	}

	Enum* p = create_element<Enum>(lexer, name);

	meger_tokens(lexer, lexer.begin(), lexer.pos(), p->decl);

	try {
		throw_parse_error_if( lexer.token().type!='{' );
		lexer.next();

		parse_enum_iterms(lexer, p->scope);
		
		throw_parse_error_if( lexer.token().type!='}' );
		lexer.next();

	} catch(ParseError&) {

#ifdef SHOW_PARSE_DEBUG_INFOS
			std::cerr << "    FilePos  : " << lexer.block().filename() << ':' << lexer.block().start_line() << std::endl;
			std::cerr << "    ";	lexer.block().dump(std::cerr) << std::endl << std::endl;
#endif

	}
	
	scope_insert(scope, p);
}

void parse_enum(BlockLexer& lexer, Scope& scope) {
	try {
		do_parse_enum(lexer, scope);
	} catch(ParseError& e) {
		parse_trace("Error when parse_enum!");
		throw e;
	}
}

