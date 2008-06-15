// cps_fun.c
// 

// TODO : not finished!!!! moreeeeeeeeeeeeeee bugsssssss!!!
// 



#include "cps_utils.h"


static MLToken* parse_function_prefix(MLToken* ps, MLToken* pe) {
	for( ; ps < pe; ++ps ) {
		switch( ps->type ) {
		case KW_EXTERN:
			if( ((ps+1) < pe) && ((ps+1)->type==TK_STRING) )
				++ps;
			break;

		case KW_STATIC:
		case KW_INLINE:
		case KW_VIRTUAL:
		case KW_EXPLICIT:
		case KW_FRIEND:
			break;

		default:
			return ps;
		}
	}

	return ps;
}

static MLToken* parse_function_args(MLToken* ps, MLToken* pe, CppElem* fun, gboolean need_save_args) {
	MLToken* retval = 0;
	MLToken* decl_start;
	TinyStr* typekey;
	gint dt;
	TinyStr* nskey = 0;
	MLToken* name = 0;

	while( ps < pe ) {
		if( ps->type==')' ) {
			break;

		} else if( ps->type==SG_ELLIPSIS ) {
			++ps;
			break;
		}

		decl_start = ps;
		typekey = 0;
		dt = KD_UNK;
		err_goto_finish_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );
		err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &dt))==0 );
		err_goto_finish_if_not( ps < pe );

		if( ps->type=='(' ) {
			// try parse function pointer
			err_goto_finish_if( (ps = skip_pair_round_brackets(ps+1, pe))==0 );
			err_goto_finish_if( ps->type!='(' );
			err_goto_finish_if( (ps = skip_pair_round_brackets(ps+1, pe))==0 );
			while( (ps < pe) && ps->type!=',' && ps->type!=')' )
				++ps;
			continue;
		}

		// try find arg name
		name = 0;
		nskey = 0;
		if( ps->type!=',' && ps->type!=')' && ps->type!='=' )
			err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

		if( need_save_args ) {
			// TODO :
			/*
			Var* p;
			size_t pos = name.find_last_of('.');
			if( pos==name.npos ) {
				p = create_element<Var>(lexer, name);
			} else {
				p = create_element<Var>(lexer, name.substr(pos));
				name.erase(pos);
				p->nskey = name;
			}
			p->typekey = ns;

			meger_tokens(lexer, start_pos, lexer.pos(), p->decl);
			scope_insert(fun.impl, p);
			*/
			g_free(typekey);
			g_free(nskey);
			typekey = 0;
			nskey = 0;
		}

		err_goto_finish_if_not( ps < pe );
		// int a[]
		if( ps->type=='[' )
			err_goto_finish_if( (ps = skip_pair_square_bracket(ps+1, pe))==0 );

		// int a = xxx;
		err_goto_finish_if_not( ps < pe );
		if( ps->type=='=' )
			err_goto_finish_if( (ps = parse_value(ps+1, pe))==0 );

		// void foo(int a, int b = 5);
		err_goto_finish_if_not( ps < pe );
		if( ps->type==',' ) {
			++ps;
			continue;
		} else {
			break;
		}
	}

	if( (ps < pe) && (ps->type==')') )
		retval = ps + 1;
	else
		retval = 0;

__cps_finish__:
	g_free(typekey);
	g_free(nskey);
	return retval;
}

static gboolean parse_function_common(Block* block, CppElem* parent, MLToken* start, TinyStr* typekey, TinyStr* nskey, MLToken* name) {
	MLToken* ps = start;
	MLToken* pe = block->tokens + block->count;
	CppElem* elem = g_new0(CppElem, 1);
	elem->type = CPP_ET_FUN;
	elem->v_fun.typekey = typekey;
	typekey = 0;

	if( nskey->len==name->len ) {
		elem->name = nskey;
	} else {
		elem->name = tiny_str_new(name->buf, name->len);
		elem->v_fun.nskey = nskey;
	}
	nskey = 0;

	err_goto_finish_if_not( (ps < pe) && ps->type=='(' );

	err_goto_finish_if( (ps = parse_function_args(ps+1, pe, elem, block->style==BLOCK_STYLE_BLOCK))==0 );

	while( (ps < pe) && ps->type!=';' && ps->type!=':' && ps->type!='{' )
		++ps;

	elem->decl = block_meger_tokens(block->tokens, ps, 0);

	//scope_insert(scope, ptr.release());
	{
		cpp_elem_clear(elem);
		g_free(elem);
	}

	if( block->style==BLOCK_STYLE_BLOCK ) {
		while( (ps < pe) && ps->type!='{' )
			++ps;

		if( ps < pe )
			;//parse_impl_scope(lexer, p->impl);
	}
	return TRUE;

__cps_finish__:
	g_free(typekey);
	g_free(nskey);
	return FALSE;
}

gboolean cps_fun(Block* block, CppElem* parent) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	TinyStr* typekey = 0;
	gint dt = KD_UNK;
	gint prdt = KD_UNK;
	TinyStr* nskey = 0;
	MLToken* name;

	err_return_false_if( (ps = parse_function_prefix(ps, pe))==0 );

	err_return_false_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );

	prdt = dt;
	err_return_false_if( (ps = parse_ptr_ref(ps, pe, &prdt))==0 );

	err_return_false_if_not( ps < pe );
	if( ps->type=='(' ) {
		// try parse function pointer
		MLToken* fptrypos = ps;
		err_return_false_if_not( (++ps) < pe );

		while( ps->type==TK_ID ) {
			err_return_false_if_not( (++ps) < pe );
			if( ps->type=='<' )
				err_return_false_if( (ps = skip_pair_angle_bracket(ps+1, pe))==0 );

			err_return_false_if_not( ps < pe );
			if( ps->type!=SG_DBL_COLON ) {
				ps = fptrypos;
				break;
			}
		}

		if( ps->type=='*' && ((ps+1) < pe) && ((ps+1)->type==TK_ID) ) {
			// function pointer
			name = ++ps;

			++ps;
			err_return_false_if_not( (ps < pe) && (ps->type==')') );
			++ps;
	
		} else {
			// no return type function
			err_return_false_if( typekey==0 || typekey->len==0 );

			if( parent->type==CPP_ET_CLASS ) {
				name = ps;

			} else {
				err_return_false_if_not( (gsize)(nskey->len) > ps->len );
				err_return_false_if_not( nskey->buf[nskey->len - (ps->len + 1)]=='.' );
				nskey->len -= (ps->len + 1);
				nskey->buf[nskey->len] = '\0';
				name = ps;
			}

			/*
			if( name==ns ) {
				// constructor

			} else if( ns.size()==(name.size() + 1) && ns[0]=='~' && ns.compare(1, ns.size()-1, name)==0 ) {
				// destructor

			} else {
				throw_parse_error("Error when parse_function!");
			}
			*/

			ps = fptrypos;
		}

	} else {
		// normal function
		err_return_false_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );
	}

	return parse_function_common(block, parent, ps, typekey, nskey, name);
}

gboolean cps_operator(Block* block, CppElem* parent) {
	if( !cps_fun(block, parent) ) {
		err_trace("parse operator function failed!");
		return FALSE;
	}
	return TRUE;
}

gboolean cps_destruct(Block* block, CppElem* parent) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	TinyStr* nskey = 0;
	MLToken* name;

	err_return_false_if( (ps = parse_function_prefix(ps, pe))==0 );

	err_return_false_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

	if( !parse_function_common(block, parent, ps, 0, nskey, name) ) {
		err_trace("parse destruct function failed!");
		return FALSE;
	}
	return TRUE;
}

