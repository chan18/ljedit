// blocklexer.h
// 

#ifndef LJCS_CPS_BLOCKLEXER_H
#define LJCS_CPS_BLOCKLEXER_H

#include "../crt_debug.h"

#include <iostream>
//#include <stdexcept>

struct ParseError {};

#ifdef SHOW_PARSE_DEBUG_INFOS

	#define throw_parse_error(reason) { \
		std::cerr << "ParseError(" << __FILE__ << ':' << __LINE__ << ')' << std::endl \
				  << "    Function : " << __FUNCTION__ << std::endl \
				  << "    Reason   : " << (reason) << std::endl; \
			throw ParseError(); \
		}

	#define throw_parse_error_if(condition) if(condition) throw_parse_error(#condition)

	#define parse_warning(reason) { \
		std::cerr << "ParseWarning(" << __FILE__ << ':' << __LINE__ << ')' << std::endl \
				  << "    Function : " << __FUNCTION__ << std::endl \
				  << "    Reason   : " << (reason) << std::endl; \
		}

	#define parse_warning_if(condition) if(condition) parse_warning(#condition)

	#define parse_trace(msg) \
		std::cerr << "    Trace    : " << msg << std::endl;

#else

	#define throw_parse_error(reason) throw ParseError();
	#define throw_parse_error_if(condition) if(condition) throw_parse_error(#condition)
	#define parse_warning(reason)
	#define parse_warning_if(condition)
	#define parse_trace(msg)

#endif

#include "../token.h"
#include "../ds.h"

using cpp::Scope;

struct LexerEnviron {
	LexerEnviron(cpp::File& f)
		: file(f) {}

	cpp::File&	file;
	Tokens		tokens;
};

class ParseNextBlockLexer;

class Block {
	friend class ParseNextBlockLexer;
public:
	Block(LexerEnviron& env)
		: env_(env)
		, begin_(0)
		, end_(0) {}

	Block(LexerEnviron& env, size_t begin, size_t end)
		: env_(env)
		, begin_(begin)
		, end_(end) {}

	bool exist() const { return begin_ < end_; }

	void set_block(size_t begin, size_t end) {
		begin_ = begin;
		end_ = end;
		assert(begin_ < end_);
	}

	bool in_block(size_t idx) const
		{ return (idx >= begin()) && (idx < end()); }

	const LexerEnviron& env() const     { return env_; }
	LexerEnviron& env()                 { return env_; }
	const Tokens& tokens() const        { return env_.tokens; }
	Tokens& tokens()                    { return env_.tokens; }

	const std::string& filename() const { return env_.file.filename; }

	size_t start_line() const {
		assert( in_block(begin()) );
		return tokens()[begin()].line;
	}

	size_t end_line() const {
		return (end() > begin()) ? tokens()[end() - 1].line : start_line();
	}

	size_t begin() const   { return begin_; }
	size_t end() const     { return end_; }

	size_t find(int type) const {
		size_t pos = begin();
		for( ; pos!=end(); ++pos )
			if( tokens()[pos].type==type )
				break;
		return pos;
	}

	std::ostream& dump(std::ostream& os) const {
		os << "Block    : [";
		Tokens::const_iterator it = tokens().begin() + begin();
		Tokens::const_iterator eit = tokens().begin() + end();
		for( ; it!=eit; ++it )
			os << ' ' << it->word;
		return os << " ]";
	}

public:
	std::string		tag;

private:
	LexerEnviron&   env_;
	size_t          begin_;
	size_t          end_;
};

class BlockLexer {
public:
	BlockLexer(Block& block)
		: block_(block)
		, pos_(block.begin())
		, view_(cpp::NORMAL_VIEW) {}

	cpp::File& file() { return block_.env().file; }

public:	// for parse
	const Block& block() const { return block_; }
	Block& block()             { return block_; }
	size_t begin() const       { return block_.begin(); }
	size_t pos() const         { return pos_; }

	void set_view(char view)   { view_ = view; }
	char view() const          { return view_; }

	void reset() { pos(block().begin()); }
	void clear() { pos_ = block().end(); }

	Token& token(size_t idx) {
		throw_parse_error_if( !block().in_block(idx) );
		return block().tokens()[idx];
	}

	Token& token() { return token(pos()); }

	Token& token_next(size_t n = 1)	{ return token(pos() + n); }
	Token& token_back(size_t n = 1)	{ return token(pos() - n); }

	Token& pos(size_t idx) {
		Token& result = token(idx);
		pos_ = idx;
		return result;
	}

	Token& next(size_t n = 1)		{ return pos(pos() + n); }
	Token& back(size_t n = 1)		{ return pos(pos() - n); }

private:
	Block&  block_;
	size_t  pos_;
	char    view_;
};

template<class T>
inline T* create_element(BlockLexer& lexer, const std::string& name) {
	return cpp::Element::create<T>( lexer.file()
		, name
		, lexer.block().start_line()
		, lexer.block().end_line()
		, lexer.view() );
}

#endif//LJCS_CPS_BLOCKLEXER_H

