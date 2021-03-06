// cps_var.c
// 

#include "cps_utils.h"

gboolean cps_var(ParseEnv* env, Block* block) {
	gboolean retval = FALSE;
	gboolean need_parse_next = TRUE;
	CppElem* elem = 0;
	gint dt = KD_UNK;
	gint prdt = KD_UNK;
	TinyStr* dtdecl = 0;
	TinyStr* typekey = 0;
	TinyStr* nskey = 0;
	MLToken* name = 0;
	MLToken* start;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	err_goto_finish_if_not( ps < pe );

	if( ps->type==';' )
		return TRUE;

	err_goto_finish_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );
	dtdecl = block_meger_tokens(block->tokens, ps, 0);

	while( need_parse_next && (ps < pe) ) {
		need_parse_next = FALSE;
		start = ps;
		prdt = dt;
		err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &prdt))==0 );
		err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

		elem = cpp_elem_new();
		elem->type = CPP_ET_VAR;
		elem->file = block->parent->file;
		if( name->len==tiny_str_len(nskey) ){
			elem->name = nskey;
		} else {
			elem->name = tiny_str_new(name->buf, name->len);
			elem->v_var.nskey = nskey;
		}
		elem->sline = name->line;
		elem->eline = name->line;
		elem->decl = block_meger_tokens(start, ps, dtdecl);
		elem->v_var.typekey = typekey;

		retval = TRUE;
		nskey = 0;
		typekey = 0;

		if( ps < pe ) {
			if( ps->type==':' )			// int a:32;
				; // parse num if need, now ignore

			else if( ps->type=='[' )	// int a[]
				err_goto_finish_if( (ps = skip_pair_square_bracket(ps+1, pe))==0 );
		}

		if( (ps < pe) && ps->type=='=' )	// int a = xxx;
			err_goto_finish_if( (ps = parse_value(ps+1, pe))==0 );

		if( (ps < pe) && ps->type==',' ) {	// int a, b = 5, c;
			++ps;
			if( elem->v_var.typekey )
				typekey = tiny_str_copy(elem->v_var.typekey);
			need_parse_next = TRUE;
		}

		cpp_scope_insert(block->parent, elem);
		elem = 0;
	}

__cps_finish__:
	cpp_elem_free(elem);

	tiny_str_free(typekey);
	tiny_str_free(nskey);
	tiny_str_free(dtdecl);
	return retval;
}

