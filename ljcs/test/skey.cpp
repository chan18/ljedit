// searcher.cpp
// 
#include "crt_debug.h"

#include "skey.h"

#include <sstream>
#include <algorithm>

void iter_skip_pair(IDocIter& it, char sch, char ech) {
	char ch = '\0';
	int layer = 1;
	while( layer > 0 ) {
		ch = it.prev();
		switch( ch ) {
		case '\0':
		case ';':
		case '{':
		case '}':
			return;
		case ':':
			if( it.prev_char()!=':' )
				return;
			break;
		default:
			if( ch==ech )
				++layer;
			else if( ch==sch )
				--layer;
			break;
		}	
	}
}

bool find_key(std::string& key, IDocIter& ps, IDocIter& pe, bool find_startswith) {
	std::ostringstream oss;
	if( !find_startswith )
		oss << '$';

	char ch = ps.prev();
	switch( ch ) {
	case '\0':
		return false;
	case '.':
		if( ps.prev_char()=='.' )
			return false;
		break;
	case '>':
		if( ps.prev()!='-' )
			return false;
		oss << ch;
		ch = '-';
		break;
	case '(':
	case '<':
		break;
	case ':':
		if( ps.prev()!=':' )
			return false;
		oss << ch;
		break;
	default:
		if( ch <= 0 )
			return false;

		if( ch!='_' && !isalnum(ch) )
			return false;
	}
	oss << ch;

	bool loop_sign = true;
	bool no_word = true;
	while( loop_sign && ((ch=ps.prev()) != '\0') ) {
		switch( ch ) {
		case '.':
			if( no_word )
				return false;
			no_word = true;
			oss << ch;
			break;

		case ':':
			if( no_word )
				return false;
			no_word = true;

			if( ps.prev_char()==':' ) {
				ps.prev();
				oss << ch << ch;
			} else {
				loop_sign = false;
			}
			break;

		case ']':
			iter_skip_pair(ps, '[', ']');
			oss << ']' << '[';
			break;
			
		case ')':
			iter_skip_pair(ps, '(', ')');
			oss << ')' << '(';
			break;
			
		case '>':
			if( no_word ) {
				if( ps.prev_char() != '>' ) {
					iter_skip_pair(ps, '<', '>');
					oss << '>' << '<';
				}
				else {
					loop_sign = false;
				}

			} else {
				if( ps.prev_char()=='-' ) {
					ps.prev();
					oss << '>' << '-';
					
				} else {
					loop_sign = false;
				}
			}
			break;
			
		default:
			if( ch>0 && (::isalnum(ch) || ch=='_') ) {
				oss << ch;
				no_word = false;
			} else {
				loop_sign = false;
			}
		}
	}

	key = oss.str();
	std::reverse(key.begin(), key.end());

	ps.next();
	return true;
}

bool parse_key(std::string& key, const std::string& text, bool find_startswith) {
	class ParseKeyIter : public IDocIter {
	public:
		ParseKeyIter(const std::string& text)
			: IDocIter('\0')
			, text_(text)
			, pos_(text.size()) {}

	protected:
		virtual char do_prev()
			{ return pos_ > 0 ? text_[--pos_] : '\0'; }

		virtual char do_next()
			{ return pos_ < text_.size() ? text_[++pos_] : '\0'; }

	private:
		const std::string&	text_;
		size_t				pos_;
	};

	if( text.empty() )
		return false;

	ParseKeyIter ps(text);
	ParseKeyIter pe(text);
	return find_key(key, ps, pe, find_startswith);
}

