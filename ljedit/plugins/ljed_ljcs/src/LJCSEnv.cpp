// LJCSEnv.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "LJCSEnv.h"


LJCSEnv& LJCSEnv::self() {
	static LJCSEnv me_;
	return me_;
}

LJCSEnv::LJCSEnv() {
	pthread_rwlock_init(&stree_rwlock, 0);
}

LJCSEnv::~LJCSEnv() {
	pthread_rwlock_destroy(&stree_rwlock);
}
