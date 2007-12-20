// ml.h
// 
#ifndef LJCS_ML_H
#define LJCS_ML_H

#include "ds.h"
#include "token.h"

#include <string>
#include <istream>
#include <ostream>

// implements in ml.l
// 
class MacroMgr;

class Lexer {
protected:
	Lexer(MacroMgr& env, cpp::File& file, std::istream* managed, const void* tag)
		: env_(env)
		, file_(file)
		, tag_(tag)
		, managed_(managed) {}

public:
	static Lexer* create(MacroMgr& env, cpp::File& file, std::istream& ins, std::ostream* out=0, const void* tag=0);

	static Lexer* create(MacroMgr& env, cpp::File& file, std::ostream* out=0, const void* tag=0);

	static Lexer* create(MacroMgr& env
		, cpp::File& file
		, const char* text
		, size_t len
		, int sline = 1
		, std::ostream* out=0
		, const void* tag=0);

	virtual ~Lexer() { delete managed_; }

	int next_token() { do_next_token(); return token().type; }

	bool eof() const					{ return token().type==0; }

	const Token& token() const			{ return token_; }

	cpp::File& file()					{ return file_; }
	const cpp::File& file() const		{ return file_; }

	const std::string& filename() const	{ return file_.filename; }

private:
	virtual void do_next_token() = 0;

protected:
	MacroMgr&		env_;
	cpp::File&		file_;
	const void*		tag_;

	Token			token_;
	std::istream*	managed_;
};

#endif//LJCS_ML_H

