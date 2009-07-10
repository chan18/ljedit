// ds.h
// 
#ifndef PUSS_CPP_DS_H
#define PUSS_CPP_DS_H

#include "debug.h"
#include "guide.h"

TinyStr* tiny_str_new(const gchar* buf, gsize len);
void     tiny_str_free(TinyStr* str);
#define  tiny_str_len(str) ((((guint)(str)->len_hi)<<8) + (guint)((str)->len_lo))
#define  tiny_str_copy(str) tiny_str_new((str)->buf, tiny_str_len(str))
gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b);
guint    tiny_str_hash(const TinyStr* v);
#define  tiny_str_len_equal(a,b) ((a->len_hi==b->len_hi) && (a->len_lo==b->len_lo))

CppElem* cpp_elem_new();
void cpp_elem_free(CppElem* elem);
void cpp_elem_clear(CppElem* elem);

void cpp_scope_insert(CppElem* parent, CppElem* elem);

void cpp_file_clear(CppFile* file);

#ifdef _DEBUG
	extern gint cps_debug_file_count;
	extern gint cps_debug_elem_count;
	extern gint cps_debug_tinystr_count;

	#define DEBUG_FILE_INC() g_atomic_int_add(&cps_debug_file_count, 1)
	#define DEBUG_FILE_DEC() g_atomic_int_add(&cps_debug_file_count, -1)

	#define DEBUG_ELEM_INC() g_atomic_int_add(&cps_debug_elem_count, 1)
	#define DEBUG_ELEM_DEC() g_atomic_int_add(&cps_debug_elem_count, -1)

	#define DEBUG_TINYSTR_INC() g_atomic_int_add(&cps_debug_tinystr_count, 1)
	#define DEBUG_TINYSTR_DEC() g_atomic_int_add(&cps_debug_tinystr_count, -1)
#else

	#define DEBUG_FILE_INC()
	#define DEBUG_FILE_DEC()

	#define DEBUG_ELEM_INC()
	#define DEBUG_ELEM_DEC()

	#define DEBUG_TINYSTR_INC()
	#define DEBUG_TINYSTR_DEC()
#endif

#endif//PUSS_CPP_DS_H

