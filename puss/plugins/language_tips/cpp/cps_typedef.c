// cps_typedef.c
// 

#include "cps_utils.h"

gboolean cps_fun(ParseEnv* env, Block* block);

static gboolean cps_normal_typedef(ParseEnv* env, Block* block) {
	TinyStr* typekey = 0;
	gint dt = KD_UNK;
	gint dtptr;
	MLToken* name;
	CppElem* elem = 0;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	err_goto_finish_if_not( (ps < pe) && ps->type==KW_TYPEDEF );
	++ps;

	switch( ps->type ) {
	case KW_STRUCT:
	case KW_CLASS:
	case KW_UNION:
		// typedef struct A B;
		++ps;
		err_goto_finish_if_not( (ps < pe) && ps->type==TK_ID );
		break;
	}

	err_goto_finish_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );
	dtptr = dt;
	err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &dtptr))==0 );

	err_goto_finish_if_not( ps < pe );
	switch( ps->type ) {
	case TK_ID:
		{
			// typedef A B;
			name = ps;

			++ps;
			while( (ps < pe) && ps->type=='[' )
				err_goto_finish_if( (ps = skip_pair_square_bracket(ps + 1, pe))==0 );

			elem = cpp_elem_new();
			elem->type = CPP_ET_TYPEDEF;
			elem->file = block->parent->file;
			elem->name = tiny_str_new(name->buf, name->len);
			elem->sline = name->line;
			elem->eline = name->line;
			elem->v_typedef.typekey = typekey;
			elem->decl = block_meger_tokens(block->tokens, ps, 0);

			typekey = 0;

			cpp_scope_insert(block->parent, elem);
		}
		break;

	default:
		// typedef (*TFn)(...);
		// typedef (T::*TFn)(...);
		err_goto_finish_if( ps->type!='(' );
		{
			CppElem tpscope;
			CppElem* tpparent = block->parent;
			memset(&tpscope, 0, sizeof(tpscope));
			tpscope.type = CPP_ET_NCSCOPE;
			tpscope.file = block->parent->file;

			--(block->count);
			++(block->tokens);
			block->parent = &tpscope;
			if( cps_fun(env, block) ) {
				if( tpscope.v_ncscope.scope ) {
					elem = tpscope.v_ncscope.scope->data;
					tpscope.v_ncscope.scope->data = 0;
					if( elem && elem->type==CPP_ET_FUN) {
						// TODO : use elem directly
						CppElem* tpelem = cpp_elem_new();
						tpelem->type = CPP_ET_TYPEDEF;
						tpelem->file = block->parent->file;
						tpelem->sline = elem->sline;
						tpelem->eline = elem->eline;

						tpelem->name = elem->name;
						elem->name = 0;

						tpelem->decl = tiny_str_new("typedef ", 8 + tiny_str_len(elem->decl));
						memcpy(tpelem->decl->buf + 8, elem->decl->buf, tiny_str_len(elem->decl));

						tpelem->v_typedef.typekey = elem->v_fun.typekey;
						elem->v_fun.typekey = 0;

						cpp_scope_insert(tpparent, tpelem);
					}
				}
			}
			block->parent = tpparent;
			cpp_elem_clear(&tpscope);
		}
		break;
	}

__cps_finish__:
	tiny_str_free(typekey);
	return TRUE;
}

static gboolean cps_complex_typedef(ParseEnv* env, Block* block) {
	// typedef struct { ... } T, *PT;
	TinyStr* str;
	CppElem* elem = 0;
	CppElem tpscope;
	GList*  node;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	err_return_false_if_not( (ps < pe) && ps->type==KW_TYPEDEF );
	++ps;

	memset( &tpscope, 0, sizeof(tpscope) );
	tpscope.file = block->parent->file;
	tpscope.type = CPP_ET_NCSCOPE;

	ps = parse_scope(env, ps, (pe - ps), &tpscope, TRUE);
	//err_goto_finish_if( ps==0 );
	node = tpscope.v_ncscope.scope;
	err_goto_finish_if( node==0 );

	elem = (CppElem*)(node->data);
	node->data = 0;
	err_goto_finish_if( elem->type!=CPP_ET_CLASS && elem->type!=CPP_ET_ENUM );

	cpp_scope_insert(block->parent, elem);

	node = g_list_next(node);
	for( ; node; node = g_list_next(node) ) {
		elem = (CppElem*)(node->data);
		node->data = 0;
		err_goto_finish_if( elem->type != CPP_ET_VAR );

		str = elem->v_var.typekey;
		tiny_str_free(elem->v_var.nskey);
		
		elem->type = CPP_ET_TYPEDEF;
		elem->v_typedef.typekey = str;

		str = elem->decl;
		elem->decl = tiny_str_new(0, 8 + tiny_str_len(str));
		memcpy(elem->decl->buf, "typedef ", 8);
		memcpy(elem->decl->buf + 8, str->buf, tiny_str_len(str));
		tiny_str_free(str);

		cpp_scope_insert(block->parent, elem);
	}

__cps_finish__:
	node = tpscope.v_ncscope.scope;
	for( ; node; node = g_list_next(node) ) {
		elem = (CppElem*)(node->data);
		if( !elem )
			continue;

		cpp_elem_free(elem);
	}

	return TRUE;
}

gboolean cps_typedef(ParseEnv* env, Block* block) {
	return block->style==BLOCK_STYLE_LINE
		? cps_normal_typedef(env, block)
		: cps_complex_typedef(env, block);
}

