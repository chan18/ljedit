// cps_typedef.c
// 

#include "cps_utils.h"

static gboolean cps_normal_typedef(Block* block, CppElem* parent) {
	TinyStr* typekey;
	gint dt = KD_UNK;
	gint dtptr;
	MLToken* name;
	CppElem* elem = 0;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	err_goto_finish_if_not( (ps < pe) && ps->type==KW_TYPEDEF );
	++ps;

	err_goto_finish_if( (ps = parse_datatype(ps, pe, &typekey, &dt))==0 );
	dtptr = dt;
	err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &dtptr))==0 );

	err_goto_finish_if_not( ps < pe );
	switch( ps->type ) {
	case KW_STRUCT:
	case KW_CLASS:
	case KW_UNION:
		// typedef struct A B;
		++ps;
		err_goto_finish_if_not( (ps < pe) && ps->type==TK_ID );

	case TK_ID:
		{
			// typedef A B;
			name = ps;

			++ps;
			while( (ps < pe) && ps->type=='[' )
				err_goto_finish_if( (ps = skip_pair_square_bracket(ps + 1, pe))==0 );

			elem = g_new(CppElem, 1);
			elem->name = tiny_str_new(ps->buf, ps->len);
			elem->v_typedef.typekey = typekey;
			elem->decl = block_meger_tokens(block->tokens, ps, 0);

			typekey = 0;
			
			//scope_insert(scope, p);
			{
				cpp_elem_clear(elem);
				g_free(elem);
			}
		}
		break;

	default:
		// TODO : 
		// typedef (*TFn)(...);
		// typedef (T::*TFn)(...);
		err_goto_finish_if( ps->type!='(' );
		break;
	}

__cps_finish__:
	g_free(typekey);
	return TRUE;
}

static gboolean cps_complex_typedef(Block* block, CppElem* parent) {
	// typedef struct { ... } T, *PT;
	TinyStr* typekey;
	MLToken* name;
	CppElem* elem = 0;
	CppElem tpscope;
	GList*  node;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	err_goto_finish_if_not( (ps < pe) && ps->type==KW_TYPEDEF );
	++ps;

	memset( &tpscope, 0, sizeof(tpscope) );
	tpscope.file = parent->file;
	tpscope.type = CPP_ET_NCSCOPE;

	parse_scope(block, &tpscope);
	node = tpscope.v_ncscope.scope;
	err_goto_finish_if( node==0 );

	elem = (CppElem*)(node->data);
	node->data = 0;
	err_goto_finish_if( elem->type!=CPP_ET_CLASS && elem->type!=CPP_ET_ENUM );
	// scope_insert(scope, elem);
	{
		cpp_elem_clear(elem);
		g_free(elem);
	}

	node = g_list_next(node);
	for( ; node; node = g_list_next(node) ) {
		elem = (CppElem*)(node->data);
		node->data = 0;
		err_goto_finish_if( elem->type != CPP_ET_VAR );

		/*
		// TODO :
		elem->type = CPP_ET_TYPEDEF;

		Typedef* p = create_element<Typedef>(lexer, var.name);
		p->typekey.swap(var.typekey);
		p->decl = "typedef ";
		p->decl.append(var.decl);

		scope_insert(scope, p);
		*/
	}

__cps_finish__:
	g_free(typekey);
	return TRUE;
}

gboolean cps_typedef(Block* block, CppElem* parent) {
	return block->style==BLOCK_STYLE_LINE
		? cps_normal_typedef(block, parent)
		: cps_complex_typedef(block, parent);
}

