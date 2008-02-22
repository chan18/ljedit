// module.cpp
//

#include "IPuss.h"

extern "C" {

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	g_print("* PluginEngine extends create\n");
	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	g_print("* PluginEngine extends destroy\n");
}

}

