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

bool find_key(std::string& key, IDocIter& ps, IDocIter& pe) {
	char type = 'S';
	std::string word;
	char ch = ps.prev();
	switch( ch ) {
	case '\0':
		return false;
	case '.':
		if( ps.prev_char()=='.' )
			return false;
		type = 'v';
		break;
	case '(':
	case '<':
		type = '*';
		break;
	case '>':
		if( ps.prev()!='-' )
			return false;
		type = 'v';
		break;
	case ':':
		if( ps.prev()!=':' )
			return false;
		type = '?';
		break;
	default:
		if( ch <= 0 )
			return false;

		if( ch!='_' && !isalnum(ch) )
			return false;
		word += ch;
	}

	std::ostringstream oss;
	if( type=='v' || type=='?' )
		oss << 'S' << ':';

	bool loop_sign = true;
	while( loop_sign && ((ch=ps.prev()) != '\0') ) {
		switch( ch ) {
		case '.':
			if( word.empty() )
				return false;
			oss << word << type << ':';
			word.clear();
			type = 'v';
			break;

		case ':':
			if( word.empty() )
				return false;
			oss << word << type << ':';
			word.clear();

			if( ps.prev_char()==':' ) {
				ps.prev();
				type = '?';
			} else {
				type = '\0';
			}
			break;

		case ']':
			iter_skip_pair(ps, '[', ']');
			break;
			
		case ')':
			iter_skip_pair(ps, '(', ')');
			if( type != 'v' )
				return false;
			type = 'f';
			break;
			
		case '>':
			if( word.empty() ) {
				if( ps.prev_char() != '>' )
					iter_skip_pair(ps, '<', '>');
				else
					loop_sign = false;

			} else {
				if( ps.prev_char()=='-' ) {
					ps.prev();
					oss << word << type << ':';
					word.clear();
					type = 'v';
					
				} else {
					oss << word << type << ':';
					word.clear();
					type = '\0';
					loop_sign = false;
				}
			}
			break;
			
		default:
			if( ch>0 && (::isalnum(ch) || ch=='_') ) {
				word += ch;
				break;
				
			} else {
				if( !word.empty() ) {
					if( word=="siht" ) 	// this
						oss << 'L' << ':';
					else
						oss << word << type << ':';
					word.clear();
					type = '\0';
				}
				loop_sign = false;
			}
		}
	}

	if( ch=='\0' && !word.empty() ) {
		if( word=="siht" ) 	// this
			oss << 'L' << ':';
		else
			oss << word << type << ':';
		word.clear();
		type = '\0';
	}

	if( type=='?' )
		oss << 'R' << ':';

	key = oss.str();
	std::reverse(key.begin(), key.end());

	ps.next();
	return true;
}

bool parse_key(std::string& key, const std::string& text) {
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
	return find_key(key, ps, pe);
}

