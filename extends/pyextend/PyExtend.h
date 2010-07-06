// PyExtend.h
//

#ifndef PUSS_EXTEND_INC_PYEXTEND_H
#define PUSS_EXTEND_INC_PYEXTEND_H

#include "IPuss.h"

typedef struct _PyExtend PyExtend;

PyExtend* puss_py_extend_create(Puss* app);

void  puss_py_extend_destroy(PyExtend* self);

#endif//PUSS_EXTEND_INC_PYEXTEND_H

