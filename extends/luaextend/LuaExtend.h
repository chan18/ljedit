// LuaExtend.h
//

#ifndef PUSS_EXTEND_INC_LUAEXTEND_H
#define PUSS_EXTEND_INC_LUAEXTEND_H

#include "IPuss.h"

typedef struct _LuaExtend LuaExtend;

LuaExtend* puss_lua_extend_create(Puss* app);

void  puss_lua_extend_destroy(LuaExtend* self);

#endif//PUSS_EXTEND_INC_LUAEXTEND_H

