// cps_namespace.c
// 

#include "cps_utils.h"

gboolean cps_namespace(ParseEnv* env, Block* block) {
	CppElem* elem = 0;
	MLToken* name;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	err_return_false_if_not( (ps < pe) && ps->type==KW_NAMESPACE );

	++ps;
	err_return_false_if_not( ps < pe );

	if( ps->type=='{' ) {
		// anonymous namespace
		elem = block->parent;

	} else {
		name = ps;
		err_return_false_if_not( name->type==TK_ID );

		++ps;
		err_return_false_if_not( (ps < pe) && ps->type=='{' );

		elem = cpp_elem_new();
		elem->type = CPP_ET_NAMESPACE;
		elem->file = block->parent->file;
		elem->name = tiny_str_new(name->buf, name->len);
		elem->sline = name->line;
		elem->eline = block->tokens[block->count-1].line;
		elem->decl = tiny_str_new(0, 10 + name->len);
		memcpy(elem->decl->buf, "namesapce ", 10);
		memcpy(elem->decl->buf + 10, name->buf, name->len);

		cpp_scope_insert(block->parent, elem);
	}

	++ps;
	err_return_false_if_not( ps < pe );

	parse_scope(env, ps, (pe - ps), elem, FALSE);
	return TRUE;
}

