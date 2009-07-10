// ds.c
// 

#include "ds.h"

#ifdef _DEBUG
	gint cps_debug_file_count = 0;
	gint cps_debug_elem_count = 0;
	gint cps_debug_tinystr_count = 0;
#endif

#define  tiny_str_mem_size(len) (sizeof(TinyStr) + len)

TinyStr* tiny_str_new(const gchar* buf, gsize len) {
	TinyStr* res;

	if( len > 0x0000ffff )
		len = 0x0000ffff;

	res = (TinyStr*)g_slice_alloc( tiny_str_mem_size(len) );
	DEBUG_TINYSTR_INC();

	res->len_hi = (gchar)(len >> 8);
	res->len_lo = (gchar)(len & 0xff);
	if( buf )
		memcpy(res->buf, buf, len);
	res->buf[len] = '\0';
	return res;
}

void tiny_str_free(TinyStr* str) {
	if( str ) {
		// !!! size = sizeof(short) + len + 1 = sizeof(TinyStr) + len
		// 
		DEBUG_TINYSTR_DEC();
		g_slice_free1(tiny_str_mem_size(tiny_str_len(str)), str);
	}
}

gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b) {
	if( a==b )
		return TRUE;

	if( a && b ) {
		gsize len = tiny_str_len(a);
		if( memcmp(a->buf, b->buf, len)==0 )
			return TRUE;
	}

	return FALSE;
}

guint tiny_str_hash(const TinyStr* v) {
	return g_str_hash(v->buf);
}

CppElem* cpp_elem_new() {
	CppElem* res;

	res = g_slice_new0(CppElem);
	if( res )
		DEBUG_ELEM_INC();

	return res;
}

void cpp_elem_free(CppElem* elem) {
	if( elem ) {
		cpp_elem_clear(elem);
		DEBUG_ELEM_DEC();

		g_slice_free(CppElem, elem);
	}
}

void cpp_elem_clear(CppElem* elem) {
	gint i;
	if( !elem )
		return;

	tiny_str_free(elem->name);
	tiny_str_free(elem->decl);

	switch( elem->type ) {
	case CPP_ET_KEYWORD:	// CppKeyword
	case CPP_ET_UNDEF:		// CppMacroUndef
		break;
	case CPP_ET_MACRO:		// CppMacroDefine
		for( i=0; i<elem->v_define.argc; ++i )
			tiny_str_free(elem->v_define.argv[i]);
		g_slice_free1( sizeof(gpointer) * elem->v_define.argc, elem->v_define.argv );
		tiny_str_free(elem->v_define.value);
		break;
	case CPP_ET_INCLUDE:	// CppMacroInclude
		tiny_str_free(elem->v_include.filename);
		g_free(elem->v_include.include_file);
		break;
	case CPP_ET_VAR:		// CppVar
		tiny_str_free(elem->v_var.typekey);
		tiny_str_free(elem->v_var.nskey);
		break;
	case CPP_ET_FUN:		// CppFun
		tiny_str_free(elem->v_fun.typekey);
		tiny_str_free(elem->v_fun.nskey);
		//g_free(elem->v_fun.fun_template);
		break;
	case CPP_ET_ENUMITEM:	// CppEnumItem
		tiny_str_free(elem->v_enum_item.value);
		break;
	case CPP_ET_ENUM:		// CppEnum
		tiny_str_free(elem->v_enum.nskey);
		break;
	case CPP_ET_USING:		// CppUsing
		tiny_str_free(elem->v_using.nskey);
		break;
	case CPP_ET_TYPEDEF:	// CppTypedef
		tiny_str_free(elem->v_typedef.typekey);
		break;
	case CPP_ET_CLASS:		// CppClass
		tiny_str_free(elem->v_class.nskey);
		for( i=0; i<elem->v_class.inhers_count; ++i )
			tiny_str_free(elem->v_class.inhers[i]);
		g_slice_free1( sizeof(gpointer) * elem->v_class.inhers_count, elem->v_class.inhers );
		break;
	case CPP_ET_NCSCOPE:	// CppNCScope
	case CPP_ET_NAMESPACE:	// CppNamespace
		break;
	default:
		g_assert( FALSE && "bad elem type" );
	}

	switch( elem->type ) {
	case CPP_ET_NCSCOPE:
	case CPP_ET_ENUM:
	case CPP_ET_CLASS:
	case CPP_ET_NAMESPACE:
	case CPP_ET_FUN:
		g_list_foreach(elem->v_ncscope.scope, (GFunc)cpp_elem_free, 0);
		break;
	}
}

static gint cpp_elem_pos_compare(const CppElem* a, const CppElem* b) {
	if( a==b )
		return 0;

	if( a && b )
		return a->sline - b->sline;

	return a ? 1 : -1;
}

void cpp_scope_insert(CppElem* parent, CppElem* elem) {
	if( parent->v_ncscope.scope )
		parent->v_ncscope.scope = g_list_insert_sorted( parent->v_ncscope.scope, elem, (GCompareFunc)cpp_elem_pos_compare );
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
		DEBUG_FILE_DEC();
		g_slice_free(CppFile, file);
	}
}

