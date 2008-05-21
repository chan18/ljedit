// cps_using.c
// 

#include "cps_utils.h"

gboolean cps_using(Block* block, GList* scope) {
	gboolean isns = FALSE;
	TinyStr* nskey = 0;
	MLToken* name;
	CppElem* elem = 0;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	err_goto_finish_if_not( (ps < pe) && ps->type==KW_USING );
	++ps;
	err_goto_finish_if_not( ps < pe );

	if( ps->type==KW_NAMESPACE ) {
		isns = TRUE;
		++ps;
	}

	err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

	elem = g_new0(CppElem, 1);
	elem->type = CPP_ET_USING;
	elem->name = tiny_str_new(name->buf, name->len);
	elem->decl = block_meger_tokens(block->tokens, ps, 0);
	elem->v_using.isns = isns;
	elem->v_using.nskey = nskey;
	nskey = 0;

	//scope_insert(scope, p);
	{
		cpp_elem_clear(elem);
		g_free(elem);
	}
	return TRUE;

__cps_finish__:
	g_free(nskey);
	return FALSE;
}

