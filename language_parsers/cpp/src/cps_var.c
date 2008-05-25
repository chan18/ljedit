// cps_var.c
// 

#include "cps_utils.h"

gboolean cps_var(Block* block, CppElem* parent) {
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

		elem = g_new0(CppElem, 1);
		elem->type = CPP_ET_VAR;
		if( name->len==nskey->len ){
			elem->name = nskey;
		} else {
			elem->name = tiny_str_new(name->buf, name->len);
			elem->v_var.nskey = nskey;
		}
		elem->decl = block_meger_tokens(start, ps, dtdecl);
		elem->v_var.typekey = typekey;

		retval = TRUE;
		nskey = 0;
		typekey = 0;

		if( ps < pe ) {
			if( ps->type==':' )			// int a:32;
				break;

			else if( ps->type=='[' )	// int a[]
				err_goto_finish_if( (ps = skip_pair_square_bracket(ps+1, pe))==0 );
		}

		if( (ps < pe) && ps->type=='=' )	// int a = xxx;
			err_goto_finish_if( (ps = parse_value(ps+1, pe))==0 );

		if( (ps < pe) && ps->type==',' ) {	// int a, b = 5, c;
			++ps;
			if( elem->v_var.typekey )
				typekey = tiny_str_clone(elem->v_var.typekey);
			need_parse_next = TRUE;
		}

		//scope_insert(scope, p);
		{
			cpp_elem_clear(elem);
			g_free(elem);
		}

		elem = 0;
	}

__cps_finish__:
	if( elem ) {
		cpp_elem_clear(elem);
		g_free(elem);
	}

	g_free(typekey);
	g_free(nskey);
	g_free(dtdecl);
	return retval;
}

