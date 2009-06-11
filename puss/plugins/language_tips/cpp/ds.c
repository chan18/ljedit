// ds.c
// 

#include "ds.h"


#define  tiny_str_mem_size(len) (sizeof(TinyStr) + len)

TinyStr* tiny_str_new(const gchar* buf, gsize len) {
	TinyStr* res;

	if( len > 0x0000ffff )
		len = 0x0000ffff;

	//TinyStr* res = (TinyStr*)g_new(gchar, tiny_str_mem_size(len) );
	res = (TinyStr*)g_slice_alloc( tiny_str_mem_size(len) );
	res->len = len;
	if( buf )
		memcpy(res->buf, buf, len);
	res->buf[len] = '\0';
	return res;
}

void tiny_str_free(TinyStr* str) {
	if( str ) {
		// !!! size = sizeof(short) + len + 1 = sizeof(TinyStr) + len
		// 
		//g_free(str);
		g_slice_free1(tiny_str_mem_size(str->len), str);
	}
}

gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b) {
	return ( a->len==b->len )
		? (memcmp(a->buf, b->buf, a->len)==0)
		: FALSE;
}

CppElem* cpp_elem_new() {
	//return g_new0(CppElem, 1);
	return g_slice_new0(CppElem);
}

void cpp_elem_free(CppElem* elem) {
	cpp_elem_clear(elem);
	//g_free(elem);
	g_slice_free(CppElem, elem);
}

void cpp_elem_clear(CppElem* elem) {
	if( !elem )
		return;

	tiny_str_free(elem->name);
	tiny_str_free(elem->decl);

	switch( elem->type ) {
	case CPP_ET_KEYWORD:
	case CPP_ET_UNDEF:
	case CPP_ET_MACRO:
		break;
	case CPP_ET_INCLUDE:
		tiny_str_free(elem->v_include.filename);
		tiny_str_free(elem->v_include.include_file);
		break;
	case CPP_ET_VAR:
		tiny_str_free(elem->v_var.typekey);
		tiny_str_free(elem->v_var.nskey);
		break;
	case CPP_ET_FUN:
		tiny_str_free(elem->v_fun.typekey);
		tiny_str_free(elem->v_fun.nskey);
		//g_free(elem->v_fun.fun_template);
		//g_free(elem->v_fun.impl);
		break;
	case CPP_ET_ENUMITEM:
		tiny_str_free(elem->v_enum_item.value);
		break;
	case CPP_ET_ENUM:
		tiny_str_free(elem->v_enum.nskey);
		break;
	case CPP_ET_USING:
		tiny_str_free(elem->v_using.nskey);
		break;
	case CPP_ET_TYPEDEF:
		tiny_str_free(elem->v_typedef.typekey);
		break;
	case CPP_ET_CLASS:
		tiny_str_free(elem->v_class.nskey);
		break;
	case CPP_ET_NCSCOPE:
	case CPP_ET_NAMESPACE:
		break;
	}

	switch( elem->type ) {
	case CPP_ET_NCSCOPE:
	case CPP_ET_ENUM:
	case CPP_ET_CLASS:
	case CPP_ET_NAMESPACE:
		g_list_foreach(elem->v_ncscope.scope, (GFunc)cpp_elem_free, 0);
		break;
	}
}

static gint cpp_elem_pos_compare(const CppElem* a, const CppElem* b) {
	return a->sline - b->sline;
}

void cpp_scope_insert(CppElem* parent, CppElem* elem) {
	if( parent->v_ncscope.scope )
		parent->v_ncscope.scope = g_list_insert_sorted( parent->v_ncscope.scope, elem, cpp_elem_pos_compare );
	else
		parent->v_ncscope.scope = g_list_append(0, elem);
}

void cpp_file_clear(CppFile* file) {
	if( !file )
		return;

	tiny_str_free(file->filename);
	cpp_elem_clear(&(file->root_scope));
}

CppFile* cpp_file_ref(CppFile* file) {
	g_assert( file );
	g_atomic_int_inc(&(file->ref_count));
	return file;
}

void cpp_file_unref(CppFile* file) {
	g_assert( file );
	if( g_atomic_int_dec_and_test(&(file->ref_count)) ) {
		cpp_file_clear(file);
		g_free(file);
	}
}

