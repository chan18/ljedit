// module.cpp
//

#include "IPuss.h"

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	//g_print("* PluginEngine extends create\n");

	// init plugin engine

	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	//g_print("* PluginEngine extends destroy\n");

	// !!! do not use gtk widgets here
	// !!! gtk_main already exit
	// !!! use gtk_quit_add() register destroy callback

	// free resource
}

