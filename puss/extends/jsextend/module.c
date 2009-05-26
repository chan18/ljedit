// module.cpp
//

#include "IPuss.h"

#include "JsExtend.h"

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	return puss_js_extend_create(app);
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	puss_js_extend_destroy((JsExtend*)self);
}

