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
	TinyStr* typekey = 0;
	gint dt;
	TinyStr* nskey = 0;
	MLToken* name = 0;
	CppElem* arg;

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

			// TODO : now ignore this
			tiny_str_free(typekey);
			typekey = 0;
			continue;
		}

		// try find arg name
		name = 0;
		nskey = 0;
		if( ps->type!=',' && ps->type!=')' && ps->type!='=' )
			err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

		if( need_save_args && name ) {
			arg = cpp_elem_new();
			arg->type = CPP_ET_VAR;
			arg->file = fun->file;
			arg->name = nskey;
			arg->sline = name->line;
			arg->eline = name->line;
			arg->decl = block_meger_tokens(decl_start, ps, 0);
			arg->v_var.typekey = typekey;

			cpp_scope_insert(fun, arg);

		} else {
			tiny_str_free(typekey);
			tiny_str_free(nskey);
		}

		typekey = 0;
		nskey = 0;

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
	tiny_str_free(typekey);
	tiny_str_free(nskey);
	return retval;
}

static gboolean parse_function_common(ParseEnv* env, Block* block, MLToken* start, TinyStr* typekey, TinyStr* nskey, MLToken* name) {
	MLToken* ps = start;
	MLToken* pe = block->tokens + block->count;
	CppElem* elem = cpp_elem_new();

	elem->type = CPP_ET_FUN;
	elem->file = block->parent->file;
	elem->sline = name->line;
	elem->eline = block->tokens[block->count-1].line;
	elem->v_fun.typekey = typekey;
	elem->v_fun.nskey = 0;
	typekey = 0;

	if( nskey && name->len==tiny_str_len(nskey) ) {
		elem->name = nskey;

	} else if( name->type==KW_OPERATOR ) {
		parse_ns(name, pe, &(elem->name), 0);
		err_goto_finish_if( elem->name==0 );

	} else if( name->buf[0]=='_' || g_ascii_isalnum(name->buf[0]) ) {
		elem->name = tiny_str_new(name->buf, name->len);

		if( nskey ) {
			elem->v_fun.nskey = tiny_str_new(nskey->buf, tiny_str_len(nskey) - name->len - 1);
			tiny_str_free(nskey);
		}

	} else {
		elem->name = tiny_str_new("@anonymous", 10);
	}
	nskey = 0;

	err_goto_finish_if_not( (ps < pe) && ps->type=='(' );

	err_goto_finish_if( (ps = parse_function_args(ps+1, pe, elem, block->style==BLOCK_STYLE_BLOCK))==0 );

	while( (ps < pe) && ps->type!=';' && ps->type!=':' && ps->type!='{' )
		++ps;

	elem->decl = block_meger_tokens(block->tokens, ps, 0);

	cpp_scope_insert(block->parent, elem);

	if( block->style==BLOCK_STYLE_BLOCK ) {
		while( (ps < pe) && ps->type!='{' )
			++ps;

		if( ps < pe )
			parse_scope(env, ps, (pe-ps), elem, FALSE);
	}

	return TRUE;

__cps_finish__:
	tiny_str_free(typekey);
	tiny_str_free(nskey);
	cpp_elem_free(elem);
	return FALSE;
}

gboolean cps_fun(ParseEnv* env, Block* block) {
	gboolean try_parse_exports = FALSE;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	TinyStr* typekey = 0;
	gint dt = KD_UNK;
	gint prdt = KD_UNK;
	TinyStr* nskey = 0;
	MLToken* name;

	err_goto_finish_if( (ps = parse_function_prefix(ps, pe))==0 );

	name = ps;
	err_goto_finish_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );

	prdt = dt;
	err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &prdt))==0 );

	err_goto_finish_if_not( ps < pe );

	if( ps->type=='(' ) {
		// try parse function pointer
		MLToken* fptrypos = ps;
		err_goto_finish_if_not( (++ps) < pe );

		while( ps->type==TK_ID ) {
			err_goto_finish_if_not( (++ps) < pe );
			if( ps->type=='<' )
				err_goto_finish_if( (ps = skip_pair_angle_bracket(ps+1, pe))==0 );

			err_goto_finish_if_not( ps < pe );
			if( ps->type!=SG_DBL_COLON ) {
				ps = fptrypos + 1;
				break;
			} else {
				++ps;
			}
		}

		err_goto_finish_if_not( ps < pe );
		if( ps->type=='*' ) {
			// function pointer
			if( ((ps+1) < pe) && ((ps+1)->type==TK_ID) )
				name = ++ps;
			else
				name = ps;

			++ps;
			err_goto_finish_if_not( (ps < pe) && (ps->type==')') );
			++ps;
	
		} else {
			// no return type function
			if( block->parent->type!=CPP_ET_CLASS && typekey && tiny_str_len(typekey)>0 ) {
				gsize typekey_len = tiny_str_len(typekey);
				ps = fptrypos - 1;
				if( ps > name ) {
					while( ps>name && ps->type!=TK_ID )
						--ps;
					name = ps;
				}
				nskey = typekey;
				typekey = 0;
				ps = fptrypos;

			} else if( ps->type==TK_ID ) {
				err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );
				err_goto_finish_if( (ps+2) >= pe || ps->type!=')' || (ps+1)->type!='(' );
				++ps;

			} else {
				ps = fptrypos;
			}
		}

	} else if( ps && ps->type==TK_ID ) {
		try_parse_exports = TRUE;

		// normal function
		err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );
		err_goto_finish_if_not( ps < pe );
		err_goto_finish_if_not( ps->type=='(' );
	}

	return parse_function_common(env, block, ps, typekey, nskey, name);
	
__cps_finish__:
	tiny_str_free(typekey);
	tiny_str_free(nskey);

	if( try_parse_exports ) {
		try_parse_exports = FALSE;
		typekey = 0;
		nskey = 0;
		dt = KD_UNK;

		err_goto_finish_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );
		prdt = dt;
		err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &prdt))==0 );
		err_goto_finish_if_not( ps < pe );
		err_goto_finish_if_not( ps->type==TK_ID );
	
		err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );
		err_goto_finish_if_not( ps < pe );
		err_goto_finish_if_not( ps->type=='(' );

		return parse_function_common(env, block, ps, typekey, nskey, name);
	}

	return FALSE;
}

gboolean cps_operator(ParseEnv* env, Block* block) {
	if( !cps_fun(env, block) ) {
		err_trace("parse operator function failed!");
		return FALSE;
	}
	return TRUE;
}

gboolean cps_destruct(ParseEnv* env, Block* block) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	TinyStr* nskey = 0;
	MLToken* name;
	MLToken tmp;

	err_return_false_if( (ps = parse_function_prefix(ps, pe))==0 );

	err_return_false_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

	if( nskey && nskey->buf[0]=='~' ) {
		tmp.buf = nskey->buf;
		tmp.len = tiny_str_len(nskey);
		tmp.line = name->line;
		tmp.type = name->type;
		name = &tmp;
	}

	if( !parse_function_common(env, block, ps, 0, nskey, name) ) {
		err_trace("parse destruct function failed!");
		return FALSE;
	}
	return TRUE;
}

