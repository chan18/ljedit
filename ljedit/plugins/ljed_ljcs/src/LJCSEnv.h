// LJCSEnv.h
// 

#ifndef LJED_INC_LJCSENV_H
#define LJED_INC_LJCSENV_H

#include "ljcs/ljcs.h"
#include <pthread.h>


class LJCSEnv {
public:
	static LJCSEnv& self();

private:
	LJCSEnv();
	~LJCSEnv();

	LJCSEnv(const LJCSEnv& o);
	LJCSEnv& operator = (const LJCSEnv& o);

public:
	cpp::STree			stree;
	pthread_rwlock_t	stree_rwlock;
};

#endif//LJED_INC_LJCSENV_H

