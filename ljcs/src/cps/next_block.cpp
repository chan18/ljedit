// next_block.cpp
// 

#include "next_block.h"

#include "../ml.h"

class ParseNextBlockLexer {
	class EOFException {};
public:
	static bool next_block(Block& block, TParseFn& fn, Lexer* lexer=0) {
		if( lexer!=0 ) {
			block.tokens().clear();
			block.begin_ = 0;
			block.end_ = 0;
		}

		ParseNextBlockLexer self(block, lexer);
		fn = 0;
		try {
			fn = self.do_next_block();
		} catch(EOFException&) {
		}

		if( fn != 0 ) {
			block.end_ = self.next_;
			return true;
		}
		return false;
	}

	static bool next_fun_impl_block(Block& block, TParseFn& fn) {
		ParseNextBlockLexer self(block);
		fn = 0;
		try {
			fn = self.do_next_fun_impl_block();
		} catch(EOFException&) {
		}

		if( fn != 0 ) {
			block.end_ = self.next_;
			return true;
		}
		return false;
	}

private:
	ParseNextBlockLexer(Block& block, Lexer* lexer=0)
		: lexer_(lexer)
		, block_(block)
		, next_(block.begin()) {}

	TParseFn do_next_block();
	TParseFn do_next_fun_impl_block();

	size_t pos() const    { return next_; }

	void pos(size_t idx)  { next_ = idx; }

	Token& cur() {
		size_t idx = next_- 1;
		assert( (lexer_!=0) ? (idx < block_.tokens().size()) : (block_.in_block(idx)) );
		return block_.tokens()[idx];
	}

	Token& next() {
		if( lexer_!=0 ) {
			for( size_t i=block_.tokens().size(); i<=next_; ++i ) {
				if( lexer_->next_token()==0 )
					throw EOFException();
				block_.tokens().push_back( lexer_->token() );
			}
		} else {
			if( !block_.in_block(next_) )
				throw EOFException();
		}
		Token& result = block_.tokens()[next_];
		++next_;
		return result;
	}

	bool skip(int token) {
		if( next().type==token )
			return true;
		--next_;
		return false;
	}

	void skip_pair_aaa() {
		int layer = 1;
		while( layer > 0 ) {
			switch( next().type ) {
			case '(':	++layer;	break;
			case ')':	--layer;	break;
			case '{':
			case '}':
			case ';':	layer = 0;	break;
			}
		}
		--next_;
	}

	void skip_pair_bbb() {
	 	int layer = 1;
	 	while( layer > 0 ) {
	 		switch( next().type ) {
			case '<':	++layer;	break;
			case '>':	--layer;	break;
			case '{':
			case '}':
			case ';':	layer = 0;	break;
			case '(':	skip_pair_aaa();	break;
			}
		}
		--next_;
	}

private:
	Lexer*     lexer_;
	Block&     block_;
	size_t     next_;
};

bool next_block(Block& block, TParseFn& fn, Lexer* lexer)
	{ return ParseNextBlockLexer::next_block(block, fn, lexer); }

bool next_fun_impl_block(Block& block, TParseFn& fn)
	{ return ParseNextBlockLexer::next_fun_impl_block(block, fn); }

#include "utils.h"

void parse_var(BlockLexer& lexer, Scope& scope);
void parse_using(BlockLexer& lexer, Scope& scope);
void parse_namespace(BlockLexer& lexer, Scope& scope);
void parse_typedef(BlockLexer& lexer, Scope& scope);
void parse_template(BlockLexer& lexer, Scope& scope);
void parse_function(BlockLexer& lexer, Scope& scope);
void parse_destruct(BlockLexer& lexer, Scope& scope);
void parse_operator(BlockLexer& lexer, Scope& scope);
void parse_extern_template(BlockLexer& lexer, Scope& scope);
void parse_extern_scope(BlockLexer& lexer, Scope& scope);
void parse_class(BlockLexer& lexer, Scope& scope);
void parse_enum(BlockLexer& lexer, Scope& scope);
void parse_block(BlockLexer& lexer, Scope& scope);
void parse_impl_block(BlockLexer& lexer, Scope& scope);

void parse_fun_or_var(BlockLexer& lexer, Scope& scope) {
	if( lexer.block().tag=="{" ) {
		parse_function(lexer, scope);

	} else {
		Scope tmp;
		try {
			parse_function(lexer, tmp);
			scope_insert_elems(scope, tmp.elems);
			tmp.elems.clear();

		} catch(ParseError&) {
			parse_trace("ingore parse_function, use parse_var!");
#ifdef SHOW_PARSE_DEBUG_INFOS
		std::cerr << "    FilePos  : " << lexer.block().filename() << ':' << lexer.block().start_line() << std::endl;
		std::cerr << "    ";	lexer.block().dump(std::cerr) << std::endl;
#endif
			lexer.reset();
			parse_var(lexer, scope);
		}
	}
}

void parse_class_or_var(BlockLexer& lexer, Scope& scope) {
	Scope tmp;
	try {
		parse_class(lexer, tmp);
		scope_insert_elems(scope, tmp.elems);
		tmp.elems.clear();

	} catch(ParseError&) {
		parse_trace("ingore parse_class, use parse_var!");
#ifdef SHOW_PARSE_DEBUG_INFOS
		std::cerr << "    FilePos  : " << lexer.block().filename() << ':' << lexer.block().start_line() << std::endl;
		std::cerr << "    ";	lexer.block().dump(std::cerr) << std::endl;
#endif
		lexer.reset();
		parse_var(lexer, scope);
	}
}

void parse_throw_breakout(BlockLexer& lexer, Scope& scope) {
	throw BreakOutError();
}

void parse_label(BlockLexer& lexer, Scope& scope) {
	switch( lexer.token().type ) {
	case KW_PUBLIC:		lexer.set_view(PUBLIC_VIEW);		break;
	case KW_PROTECTED:	lexer.set_view(PROTECTED_VIEW);	break;
	case KW_PRIVATE:	lexer.set_view(PRIVATE_VIEW);	break;
	}
}

TParseFn ParseNextBlockLexer::do_next_block() {
	TParseFn fn = 0;
	bool use_template = false;
	bool stop_with_blance = false;
	size_t startpos = pos();
	while( fn==0 ) {
		switch( next().type ) {
		case ';':
		case '=':	fn = &parse_var;			break;
		case '~':	fn = &parse_destruct;		stop_with_blance = true;	break;
		case '{':	fn = &parse_block;			stop_with_blance = true;	break;
		case '}':	fn = &parse_throw_breakout;	stop_with_blance = true;	break;
		case '(':	fn = use_template ? &parse_template : &parse_fun_or_var;		stop_with_blance = true;	break;
		case ':':
			if( (pos() - startpos)==2 ) {
				fn = &parse_label;
				return fn;
			} else {
				fn = &parse_skip_block;
			}
			break;
		case '<':
			skip_pair_bbb();
			if( next().type !='>' )
				fn = &parse_skip_block;
			break;
		case KW_EXPLICIT:	fn = &parse_function;	stop_with_blance = true;	break;
		case KW_USING:		fn = &parse_using;				break;
		case KW_TYPEDEF:	fn = &parse_typedef;			break;
		case KW_TEMPLATE:	use_template = true;	next();	skip_pair_bbb();	break;
		case KW_NAMESPACE:	fn = &parse_namespace;	stop_with_blance = true;	break;
		case KW_OPERATOR:	fn = use_template ? &parse_template : &parse_operator;	stop_with_blance = true;	break;
		case KW_EXTERN:
			{
				switch( next().type ) {
				case KW_TEMPLATE:
					fn = &parse_extern_template;
				case TK_STRING:
					if( next().type=='{' ) {
						fn = &parse_extern_scope;
						stop_with_blance = true;
					}
					break;
				}
			}
			break;
		case KW_STRUCT:
		case KW_CLASS:
		case KW_UNION:
			{
				size_t mark = pos();
				while( fn==0 ) {
					switch( next().type ) {
					case ':':
					case KW_PUBLIC:
					case KW_PROTECTED:
					case KW_PRIVATE:
					case '{':	fn = use_template ? &parse_template : &parse_class;			break;
					case '=':	fn = &parse_var;			break;
					case '(':	fn = use_template ? &parse_template : &parse_fun_or_var;	stop_with_blance = true;	break;
					case ';':	fn = use_template ? &parse_template : &parse_class_or_var;	break;
					}
				}
				pos(mark);
			}
			break;
		case KW_ENUM:
			fn = &parse_enum;
			break;
		case KW_ASM:			case KW_BREAK:			case KW_CASE:
		case KW_CATCH:			case KW_CONST_CAST:		case KW_CONTINUE:
		case KW_DEFAULT:		case KW_DELETE:			case KW_DO:
		case KW_DYNAMIC_CAST:	case KW_ELSE:			case KW_FOR:
		case KW_FRIEND:			case KW_GOTO:			case KW_IF:
		case KW_NEW:			case KW_REINTERPRET_CAST:
		case KW_RETURN:			case KW_STATIC_CAST:	case KW_SWITCH:
		case KW_THIS:			case KW_THROW:			case KW_TRY:
		case KW_WHILE:
			fn = &parse_skip_block;
			break;
		}
	}

	if( fn != 0 ) {
		bool has_scope = false;
		int layer = 0;
		size_t type = cur().type;
		for(;;) {
			if( type==';' ) {
				if( layer==0 )
					break;
			} else if( type=='{' ) {
				has_scope = true;
				++layer;
			} else if( type=='}' ) {
				has_scope = true;
				--layer;
				if( layer==0 && stop_with_blance )
					break;
			}
			type = next().type;
		}
		block_.tag = has_scope ? "{" : ";";
	}

	return fn;
}

TParseFn ParseNextBlockLexer::do_next_fun_impl_block() {
	TParseFn fn = 0;
	bool use_template = false;
	bool stop_with_blance_start = false;
	bool stop_with_blance = false;
	size_t startpos = pos();
	while( fn==0 ) {
		switch( next().type ) {
		case ';':
		case '=':	fn = &parse_var;			break;
		case '{':	fn = &parse_impl_block;		stop_with_blance = true;	break;
		case '}':	fn = &parse_throw_breakout;	stop_with_blance = true;	break;
		case '(':	fn = use_template ? &parse_template : &parse_fun_or_var;		stop_with_blance = true;	break;
		case ':':
			if( (pos() - startpos)==2 ) {
				fn = &parse_label;
				return fn;
			} else {
				fn = &parse_skip_block;
			}
			break;
		case '<':
			skip_pair_bbb();
			if( next().type !='>' )
				fn = &parse_skip_block;
			break;
		case KW_USING:		fn = &parse_using;		break;
		case KW_TYPEDEF:	fn = &parse_typedef;	break;
		case KW_EXTERN:		fn = &parse_function;	break;
		
		case KW_STRUCT:
		case KW_CLASS:
		case KW_UNION:
			{
				size_t mark = pos();
				while( fn==0 ) {
					switch( next().type ) {
					case ':':
					case KW_PUBLIC:
					case KW_PROTECTED:
					case KW_PRIVATE:
					case '{':	fn = use_template ? &parse_template : &parse_class;			break;
					case '=':	fn = &parse_var;			break;
					case '(':	fn = use_template ? &parse_template : &parse_fun_or_var;	stop_with_blance = true;	break;
					case ';':	fn = use_template ? &parse_template : &parse_class_or_var;	break;
					}
				}
				pos(mark);
			}
			break;
		case KW_ENUM:
			fn = &parse_enum;
			break;
			
		case KW_BREAK:						case KW_CONST_CAST:
		case KW_CONTINUE:		case KW_DELETE:			case KW_DYNAMIC_CAST:
		case KW_GOTO:			case KW_NEW:			case KW_REINTERPRET_CAST:
		case KW_RETURN:			case KW_STATIC_CAST:	case KW_THIS:
		case KW_THROW:			case KW_OPERATOR:		case KW_TEMPLATE:
			// ignore line
			fn = &parse_skip_block;
			break;
		case KW_EXPLICIT:		case KW_NAMESPACE:		case KW_FRIEND:
			// will not appead in function implement
			fn = &parse_skip_block;
			break;
		case KW_CASE:			case KW_DEFAULT:
			if( next().type==':' ) {
				fn = &parse_label;
				return fn;
			} else {
				fn = &parse_skip_block;
			}
			break;
		case KW_ELSE:			case KW_FOR:			case KW_IF:
		case KW_SWITCH:			case KW_TRY:			case KW_WHILE:
		case KW_CATCH:			case KW_DO:
			// now ignore them!
			{
				stop_with_blance_start = true;
				fn = &parse_skip_block;
			}
			break;
		}
	}

	if( fn != 0 ) {
		bool has_scope = false;
		int layer = 0;
		size_t type = cur().type;
		for(;;) {
			if( type==';' ) {
				if( layer==0 )
					break;
			} else if( type=='{' ) {
				if( layer==0 && stop_with_blance_start ) {
					pos(pos() - 1);
					break;
				} else {
					has_scope = true;
					++layer;
				}
			} else if( type=='}' ) {
				if( layer==0 )
					break;
				has_scope = true;
				--layer;
				if( layer==0 && stop_with_blance )
					break;
			}
			type = next().type;
		}
		block_.tag = has_scope ? "{" : ";";
	}

	return fn;
}

