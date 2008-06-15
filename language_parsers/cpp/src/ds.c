// ds.c
// 

#include "ds.h"


TinyStr* tiny_str_new(gchar* buf, gshort len) {
	TinyStr* res = (TinyStr*)g_new(gchar, (sizeof(TinyStr) + len));
	res->len = len;
	if( buf )
		memcpy(res->buf, buf, len);
	res->buf[len] = '\0';
	return res;
}

gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b) {
	return ( a->len==b->len )
		? (memcmp(a->buf, b->buf, a->len)==0)
		: FALSE;
}

static void cpp_elem_free(CppElem* elem, gpointer tag) {
	cpp_elem_clear(elem);
	g_free(elem);
}

void cpp_elem_clear(CppElem* elem) {
	if( !elem )
		return;

	g_free(elem->name);
	g_free(elem->decl);

	switch( elem->type ) {
	case CPP_ET_KEYWORD:
	case CPP_ET_UNDEF:
	case CPP_ET_MACRO:
		break;
	case CPP_ET_INCLUDE:
		g_free(elem->v_include.filename);
		g_free(elem->v_include.include_file);
		break;
	case CPP_ET_VAR:
		g_free(elem->v_var.typekey);
		g_free(elem->v_var.nskey);
		break;
	case CPP_ET_FUN:
		g_free(elem->v_fun.typekey);
		g_free(elem->v_fun.nskey);
		//g_free(elem->v_fun.fun_template);
		//g_free(elem->v_fun.impl);
		break;
	case CPP_ET_NCSCOPE:
		g_list_foreach(elem->v_ncscope.scope, (GFunc)cpp_elem_free, 0);
		break;
	case CPP_ET_ENUMITEM:
	case CPP_ET_ENUM:
	case CPP_ET_CLASS:
	case CPP_ET_USING:
	case CPP_ET_NAMESPACE:
	case CPP_ET_TYPEDEF:
		break;
	}
}

void cpp_file_clear(CppFile* file) {
	if( !file )
		return;

	g_free(file->filename);
	cpp_elem_clear(&(file->root_scope));
}

