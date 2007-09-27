// LJCSEnv.h
// 

#ifndef LJED_INC_LJCSENV_H
#define LJED_INC_LJCSENV_H

#include "ljcs/ljcs.h"

class LJCSEnv {
public:
	static LJCSEnv& self() {
		static LJCSEnv me_;
		return me_;
	}

private:
	LJCSEnv() {}
	~LJCSEnv() {}

	LJCSEnv(const LJCSEnv& o);
	LJCSEnv& operator = (const LJCSEnv& o);

public:
	// TODO : now just simple implement
	// 
	cpp::STree stree;
};

#endif//LJED_INC_LJCSENV_H

