// module.cpp
//

#include "IPuss.h"

#include "PyExtend.h"

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	return puss_py_extend_create(app);
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	return puss_py_extend_destroy((PyExtend*)self);
}

