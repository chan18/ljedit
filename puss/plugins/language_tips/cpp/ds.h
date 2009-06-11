// ds.h
// 
#ifndef PUSS_CPP_DS_H
#define PUSS_CPP_DS_H

#include "guide.h"

TinyStr* tiny_str_new(const gchar* buf, gsize len);
void     tiny_str_free(TinyStr* str);
#define  tiny_str_copy(str) tiny_str_new((str)->buf, (str)->len)
gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b);

CppElem* cpp_elem_new();
void cpp_elem_free(CppElem* elem);
void cpp_elem_clear(CppElem* elem);

void cpp_scope_insert(CppElem* parent, CppElem* elem);

void cpp_file_clear(CppFile* file);

#endif//PUSS_CPP_DS_H

