// ds.h
// 
#ifndef PUSS_CPP_DS_H
#define PUSS_CPP_DS_H

#include "guide.h"

TinyStr* tiny_str_new(const gchar* buf, gshort len);
void     tiny_str_free(TinyStr* str);
#define  tiny_str_copy(str) tiny_str_new((str)->buf, (str)->len)
gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b);

CppElem* cpp_elem_new();
void cpp_elem_free(CppElem* elem);
void cpp_elem_clear(CppElem* elem);

#define cpp_elem_has_subscope(e)		\
		(  (e)->type==CPP_ET_NCSCOPE	\
		|| (e)->type==CPP_ET_NAMESPACE	\
		|| (e)->type==CPP_ET_CLASS		\
		|| (e)->type==CPP_ET_ENUM		\
		|| (e)->type==CPP_ET_FUN )

#define cpp_elem_get_subscope(e) ((e)->v_ncscope.scope)

void cpp_scope_insert(CppElem* parent, CppElem* elem);

void     cpp_file_clear(CppFile* file);

CppFile* cpp_file_ref(CppFile* file);
void     cpp_file_unref(CppFile* file);

#endif//PUSS_CPP_DS_H

