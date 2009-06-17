// cps_enum.c
// 

#include "cps_utils.h"

MLToken* parse_enum_iterms(MLToken* ps, MLToken* pe, CppElem* parent) {
	CppElem* elem;
	while( (ps < pe) && ps->type==TK_ID ) {
		elem = cpp_elem_new();
		elem->type = CPP_ET_ENUMITEM;
		elem->file = parent->file;
		elem->name = tiny_str_new(ps->buf, ps->len);
		elem->decl = tiny_str_new(ps->buf, ps->len);

		cpp_scope_insert(parent, elem);

		++ps;

		if( (ps < pe) && ps->type=='=' )
			err_return_null_if( (ps = parse_value(ps + 1, pe))==0 );

		if( (ps < pe) && ps->type==',' )
			++ps;
	}

	return ps;
}

gboolean cps_enum(ParseEnv* env, Block* block) {
	CppElem* elem = 0;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	while( (ps < pe) && ps->type!=KW_ENUM )
		++ps;

	++ps;
	err_return_false_if_not( ps < pe );

	elem = cpp_elem_new();
	elem->type = CPP_ET_ENUM;
	elem->file = block->parent->file;

	if( ps->type==TK_ID ) {
		elem->name = tiny_str_new(ps->buf, ps->len);
		++ps;
	} else {
		elem->name = tiny_str_new("@anonymous", 10);
	}

	elem->decl = block_meger_tokens(block->tokens, ps, 0);

	cpp_scope_insert(block->parent, elem);

	if( (ps < pe) && ps->type=='{' ) {
		err_return_false_if( (ps = parse_enum_iterms(ps + 1, pe, elem))==0 );

		err_return_false_if_not( (ps < pe) && ps->type=='}' );
	}

	return TRUE;
}

