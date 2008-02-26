// PyExtend.h
//

#ifndef PUSS_EXTEND_INC_PYEXTEND_H
#define PUSS_EXTEND_INC_PYEXTEND_H

struct Puss;
struct PyExtend;

PyExtend* puss_py_extend_create(Puss* app);

void  puss_py_extend_destroy(PyExtend* self);

#endif//PUSS_EXTEND_INC_PYEXTEND_H

