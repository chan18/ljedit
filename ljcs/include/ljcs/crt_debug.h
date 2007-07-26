// crt_debug.h
// 

#ifndef LJCS_LJCS_CONFIG_H
#define LJCS_LJCS_CONFIG_H

#ifdef WIN32
	// for VC debug
	#define _CRTDBG_MAP_ALLOC 
	#include<stdlib.h> 
	#include<crtdbg.h> 
#endif

#endif//LJCS_LJCS_CONFIG_H

