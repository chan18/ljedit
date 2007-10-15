// utils.cpp
// 

#include "utils.h"
#include "next_block.h"
#include <sstream>

inline bool is_id_char(char ch) { return ch > 0 && (isalnum(ch) || ch=='_'); }

void meger_tokens(BlockLexer& lexer, size_t ps, size_t pe, std::string& decl) {
	std::ostringstream ss;
	ss << decl;

	bool need_space = false;
	char last = decl.empty() ? '\0' : decl[decl.size() - 1];
	char first = '\0';

	for( ; ps < pe; ++ps ) {
		Token& tkn = lexer.token(ps);
		assert( !tkn.word.empty() );

		need_space = false;
		first = tkn.word[0];
		
		if( last=='<' && first=='<' )
			need_space = true;
		else if( last=='>' && first!=':' )
			need_space = true;
		else if( (last=='*' || last=='&') && is_id_char(first) )	//!='*' && first!='&' && first!=',' && first!=')') )
			need_space = true;
		else if( last==',' && (is_id_char(first) || first=='.') )
			need_space = true;
		else if( is_id_char(last) && is_id_char(first) )
			need_space = true;
		
		if( need_space )
			ss << ' ';

		ss << tkn.word;
		last = tkn.word[ tkn.word.size() - 1];
	}

	decl = ss.str();
}

// parse ns
// i.e
//  T
//      => [T]
//  ::T
//      => [::, T]
//  std::list<int>::iterator
//      => [std, list, iterator]
//
void parse_ns(BlockLexer& lexer, std::string& ns) {
	if( lexer.token().type==SG_DBL_COLON ) {
		ns += '.';
		lexer.next();
	}

	for(;;) {
		if( lexer.token().type=='~' ) {
			throw_parse_error_if( lexer.next().type != TK_ID );
			ns += lexer.token_back().word;
			ns += lexer.token().word;
		} else if( lexer.token().type==KW_OPERATOR ) {
			ns += lexer.token().word;
			lexer.next();
			int tkn = lexer.token().type;
			if( tkn=='(' ) {
				throw_parse_error_if( lexer.next().type!=')' );
				ns += "()";
			} else if( tkn=='[' ) {
				throw_parse_error_if( lexer.next().type!=']' );
				ns += "[]";
			} else {
				std::string& s = lexer.token().word;
				if( s.size()>0 && s[0]>0 && isalpha(s[0]) )
					ns += ' ';
				ns += lexer.token().word;
			}

			if( (tkn==KW_NEW || tkn==KW_DELETE) && lexer.token_next().type=='[' ) {
				lexer.next();
				throw_parse_error_if( lexer.next().type!=']' );
				ns += "[]";
			}

		} else {
			if( lexer.token().type==KW_TEMPLATE )
				lexer.next();
			throw_parse_error_if( lexer.token().type != TK_ID );
			ns += lexer.token().word;
		}

		if( lexer.next().type=='<' ) {
			lexer.next();
			skip_pair_bbb(lexer);
		}

		if( lexer.token().type==SG_DBL_COLON ) {
			lexer.next();
			ns += '.';
			continue;
		}

		break;
	}
}

void do_parse_datatype(BlockLexer& lexer, std::string& ns, int& dt) {
	dt = KD_UNK;
	ns.clear();
	//ns.keys().clear();
	bool is_std = false;

	for(;;) {
		Token& tkn = lexer.token();
		switch( tkn.type ) {
		case KW_EXPORT:		case KW_EXTERN:		case KW_STATIC:
		case KW_AUTO:		case KW_REGISTER:	case KW_CONST:
		case KW_VOLATILE:	case KW_MUTABLE:
			break;

		case KW_VOID:		case KW_BOOL:		case KW_CHAR:
		case KW_WCHAR_T:	case KW_DOUBLE:		case KW_SHORT:
		case KW_FLOAT:		case KW_INT:		case KW_LONG:
		case KW_SIGNED:		case KW_UNSIGNED:
			dt = KD_STD;
			is_std = true;
			break;

		case TK_ID:
			if( tkn.word=="size_t" ) {
				dt = KD_STD;
				is_std = true;
				break;
				
			} else if( is_std || !ns.empty() ) {
				return;
			}
			// not use break;

		case SG_DBL_COLON:
			parse_ns(lexer, ns);
			return;

		case KW_CLASS:		case KW_STRUCT:		case KW_UNION:
		case KW_TYPENAME:	case KW_ENUM:
			if( is_std )
				return;

			dt = tkn.type;
			if( !ns.empty() )
				return;
			if( lexer.token_next().type==TK_ID ) {
				lexer.next();
				parse_ns(lexer, ns);
				return;
			}
			break;

		case KW_TEMPLATE:
			throw_parse_error("logic error, find template when parse datatype!");
			break;
			
		default:
			return;
		}

		lexer.next();
	}
}

void parse_datatype(BlockLexer& lexer, std::string& ns, int& dt) {
	try {
		do_parse_datatype(lexer, ns, dt);
	} catch( ParseError& e) {
		parse_trace("Error when parse data type!");
		throw e;
	}
}

void parse_ptr_ref(BlockLexer& lexer, int& dt) {
	for(;;) {
		switch( lexer.token().type ) {
		case KW_EXPORT:		case KW_EXTERN:		case KW_STATIC:
		case KW_INLINE:		case KW_AUTO:		case KW_REGISTER:
		case KW_CONST:		case KW_VOLATILE:	case KW_MUTABLE:
		case KW_VIRTUAL:
			break;
		case '*':
			dt = KD_PTR;
			break;
		case '&':
			dt = KD_REF;
			break;
		default:
			return;
		}
		lexer.next();
	}
}

void parse_id(BlockLexer& lexer, std::string& ns) {
	try {
		parse_ns(lexer, ns);
	} catch( ParseError& e) {
		parse_trace("Error when parse id!");
		throw e;
	}
}

void parse_value(BlockLexer& lexer) {
	try {
		for(;;) {
			int type = lexer.token().type;
			if( type==',' || type==';' || type==')' || type=='}' )
				break;

			lexer.next();
			switch( type ) {
			case '(':	skip_pair_aaa(lexer);	break;
			case '<':	skip_pair_bbb(lexer);	break;
			case '[':	skip_pair_ccc(lexer);	break;
			case '{':	skip_pair_ddd(lexer);	break;
			}
		}
		
	} catch( ParseError& e) {
		parse_trace("Error when parse value!");
		throw e;
	}
}

void parse_scope(BlockLexer& lexer, Scope& scope, Element* parent) {
	Block subblock(lexer.block().env(), parent, lexer.pos(), lexer.block().end());
	TParseFn fn = 0;
	while( next_block(subblock, fn, 0) ) {
		assert( fn != 0 );

		BlockLexer slexer(subblock);

		try {
			fn( slexer, scope );
		} catch(BreakOutError&) {
			break;
		} catch(ParseError&) {

#ifdef SHOW_PARSE_DEBUG_INFOS
			std::cerr << "    FilePos  : " << subblock.filename() << ':' << subblock.start_line() << std::endl;
			std::cerr << "    ";	subblock.dump(std::cerr) << std::endl << std::endl;
#endif

		}

		if( subblock.end()==lexer.block().end() ) {
			lexer.clear();
			break;
		}

		lexer.pos(subblock.end());
		subblock.set_block(subblock.end(), lexer.block().end());
	}
}

void parse_skip_block(BlockLexer& lexer, Scope& scope) {
	// empty implement
}

void parse_impl_scope(BlockLexer& lexer, Scope& scope) {
	Block subblock(lexer.block().env(), 0, lexer.pos(), lexer.block().end());
	TParseFn fn = 0;
	while( next_fun_impl_block(subblock, fn) ) {
		assert( fn != 0 );

		BlockLexer slexer(subblock);

		try {
			fn( slexer, scope );
		} catch(BreakOutError&) {
			break;
		} catch(ParseError&) {

#ifdef SHOW_PARSE_DEBUG_INFOS
			std::cerr << "    FilePos  : " << subblock.filename() << ':' << subblock.start_line() << std::endl;
			std::cerr << "    ";	subblock.dump(std::cerr) << std::endl << std::endl;
#endif

		}

		if( subblock.end()==lexer.block().end() ) {
			lexer.clear();
			break;
		}

		lexer.pos(subblock.end());
		subblock.set_block(subblock.end(), lexer.block().end());
	}
}

