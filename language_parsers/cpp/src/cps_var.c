// cps_var.c
// 

#include "cps_utils.h"

gboolean cps_var(Block* block, GList* scope) {
	gint dt = KD_UNK;
	gint prdt = KD_UNK;
	TinyStr* dtdecl = 0;
	TinyStr* typekey = 0;
	TinyStr* nskey = 0;
	MLToken* name = 0;

	MLToken* start;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	err_goto_error_if_not( ps < pe );

	if( ps->type==';' )
		return TRUE;

	err_goto_error_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );
	dtdecl = block_meger_tokens(block->tokens, ps, 0);

	while( ps < pe ) {
		CppElem* elem;
		start = ps;
		prdt = dt;
		err_goto_error_if( (ps = parse_ptr_ref(ps, pe, &prdt))==0 );
		err_goto_error_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

		elem = g_new0(CppElem, 1);
		if( name->len==nskey->len ){
			elem->name = nskey;
		} else {
			elem->name = tiny_str_new(name->buf, name->len);
			elem->v_var.nskey = nskey;
		}
		elem->decl = block_meger_tokens(start, ps, dtdecl);
		elem->v_var.typekey = typekey;

		nskey = 0;
		typekey = 0;

		//scope_insert(scope, p);

		if( ps < pe ) {
			if( ps->type==':' )			// int a:32;
				break;

			else if( ps->type=='[' )	// int a[]
				err_goto_error_if( (ps = skip_pair_square_bracket(ps+1, pe))==0 );
		}

		if( ps < pe ) {
			if( ps->type=='=' ) {			// int a = xxx;
				err_goto_error_if( (ps = parse_value(ps+1, pe))==0 );

			} else if( ps->type==',' ) {	// int a, b = 5, c;
				++ps;
				if( elem->v_var.typekey )
					typekey = tiny_str_new(elem->v_var.typekey->buf, elem->v_var.typekey->len);
				continue;

			} else {
				break;
			}
		}
	}

	g_free(typekey);
	return TRUE;

__cps_error__:
	g_free(typekey);
	g_free(nskey);
	return FALSE;
}

