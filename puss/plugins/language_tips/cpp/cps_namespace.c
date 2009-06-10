// cps_namespace.c
// 

#include "cps_utils.h"

gboolean cps_namespace(ParseEnv* env, Block* block) {
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

		elem = cpp_elem_new();
		elem->name = tiny_str_new(name->buf, name->len);
		elem->decl = tiny_str_new(0, 10 + name->len);
		memcpy(elem->decl->buf, "namesapce ", 10);
		memcpy(elem->decl->buf + 8, name->buf, name->len);

		cpp_scope_insert(block->parent, elem);
		parent = elem;
	}

	++ps;
	err_return_false_if_not( ps < pe );

	parse_scope(env, ps, (pe - ps), block->parent, FALSE);
	return TRUE;
}

