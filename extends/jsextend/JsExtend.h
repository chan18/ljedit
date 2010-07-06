// JsExtend.h
//

#ifndef PUSS_EXTEND_INC_JSEXTEND_H
#define PUSS_EXTEND_INC_JSEXTEND_H

#include "IPuss.h"

typedef struct _JsExtend JsExtend;

JsExtend* puss_js_extend_create(Puss* app);

void  puss_js_extend_destroy(JsExtend* self);

#endif//PUSS_EXTEND_INC_JSEXTEND_H

