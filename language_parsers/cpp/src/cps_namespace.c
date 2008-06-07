// cps_namespace.c
// 

#include "cps_utils.h"

gboolean cps_namespace(Block* block, CppElem* parent) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	err_return_false_if_not( (ps < pe) && ps->type==KW_NAMESPACE );

	++ps;
	err_return_false_if_not( ps < pe );

	if( ps->type=='{' ) {
		// anonymous namespace
		// do nothing

	} else {
		CppElem* elem;
		MLToken* name = ps;
		err_return_false_if_not( name->type==TK_ID );

		++ps;
		err_return_false_if_not( (ps < pe) && ps->type=='{' );

		elem = g_new0(CppElem, 1);
		elem->name = tiny_str_new(name->buf, name->len);
		elem->decl = tiny_str_new(0, 10 + name->len);
		memcpy(elem->decl->buf, "namesapce ", 10);
		memcpy(elem->decl->buf + 8, name->buf, name->len);
		// scope_insert(scope, p);
		parent = elem;
	}

	++ps;
	err_return_false_if_not( ps < pe );

	block->tokens = ps;
	block->count = pe - ps;

	parse_scope(block, parent);
	return TRUE;
}

