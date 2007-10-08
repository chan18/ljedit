// crt_debug.h
// 

#ifndef LJCS_LJCS_CONFIG_H
#define LJCS_LJCS_CONFIG_H

#ifdef WIN32
	#ifdef _MSC_VER
		// for VC debug
		#define _CRTDBG_MAP_ALLOC 
		#include<stdlib.h>
		#include<crtdbg.h>
	#endif
#endif

#endif//LJCS_LJCS_CONFIG_H

