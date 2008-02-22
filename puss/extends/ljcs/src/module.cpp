// module.cpp
//

#include "IPuss.h"

#include <gmodule.h>

#include "Environ.h"

extern "C" {

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	g_print("* ljcs extends create\n");
	Environ::__create(app);
	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	Environ::__destroy();
	g_print("* ljcs extends destroy\n");
}

}

