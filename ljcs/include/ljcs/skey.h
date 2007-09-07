// skey.h
// 

#ifndef LJCS_SKEY_H
#define LJCS_SKEY_H

#include <string>

class IDocIter {
public:
	IDocIter(char cur) : cur_(cur) {}
	virtual ~IDocIter() {}

	char cur() const { return cur_; }

	char prev() {
		char ch = do_prev();
		if( ch != '\0' )
			cur_ = ch;
		return ch;
	}

	char next() {
		char ch = do_next();
		if( ch != '\0' )
			cur_ = ch;
		return ch;
	}

	char prev_char() {
		char ch = do_prev();
		do_next();
		return ch;
	}

	char next_char() {
		char ch = do_next();
		do_prev();
		return ch;
	}

protected:
	virtual char do_prev() = 0;
	virtual char do_next() = 0;

private:
	char cur_;
};

bool find_key(std::string& key, IDocIter& ps, IDocIter& pe, bool find_startswith=true);

bool parse_key(std::string& key, const std::string& text, bool find_startswith=false);

#endif//LJCS_SKEY_H

