// module.cpp
//

#include "IPuss.h"

#include "LuaExtend.h"

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	return puss_lua_extend_create(app);
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	puss_lua_extend_destroy((LuaExtend*)self);
}

