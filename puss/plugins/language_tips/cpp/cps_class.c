// cps_class.c
// 

#include "cps_utils.h"

gboolean cps_var(ParseEnv* env, Block* block);

#define MAX_INHERS 256

gboolean cps_class(ParseEnv* env, Block* block) {
	CppElem* elem = 0;
	TinyStr* nskey = 0;
	MLToken* name = 0;
	gint class_type = KD_UNK;
	TinyStr* inhers[MAX_INHERS];
	gint inhers_count = 0;
	MLToken* ps;
	MLToken* pe;
	gint line;

	ps = block->tokens;
	pe = ps + block->count;
	for( ; ps < pe; ++ps ) {
		if( ps->type==KW_STRUCT || ps->type==KW_CLASS || ps->type==KW_UNION ) {
			class_type = ps->type;
			break;
		}
	}

	++ps;

	// class xxx_export CTest {};
	// 
	// fix : not find xxx_export some times
	// 
	while( ((ps+1) < pe) && ps->type==TK_ID && (ps+1)->type==TK_ID )
		++ps;

	err_goto_finish_if_not( ps < pe );

	line = ps->line;
	if( ps->type==TK_ID )
		err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

	elem = cpp_elem_new();
	elem->type = CPP_ET_CLASS;
	elem->file = env->file;
	elem->sline = line;
	elem->eline = pe->line;

	if( name ) {
		if( name->len == tiny_str_len(nskey) ) {
			elem->name = nskey;
		} else {
			elem->name = tiny_str_new(name->buf, name->len);
			elem->v_class.nskey = nskey;
		}
		nskey = 0;
			
	} else {
		gchar* str = g_strdup_printf("@anonymous_%p", elem);
		elem->name = tiny_str_new(str, strlen(str));
		g_free(str);
	}

	switch( class_type ) {
	case KW_STRUCT:	elem->v_class.class_type = CPP_CLASS_TYPE_STRUCT;	break;
	case KW_CLASS:	elem->v_class.class_type = CPP_CLASS_TYPE_CLASS;	break;
	case KW_UNION:	elem->v_class.class_type = CPP_CLASS_TYPE_UNION;	break;
	default:		elem->v_class.class_type = CPP_CLASS_TYPE_UNKNOWN;	break;
	}

	elem->decl = block_meger_tokens(block->tokens, ps, 0);

	cpp_scope_insert(block->parent, elem);

	err_goto_finish_if_not( ps < pe );

	if( ps->type==':' ) {
		// parse inheritance
		do {
			++ps;
			err_goto_finish_if_not( ps < pe );
			if( ps->type==KW_VIRTUAL ) {
				++ps;
				err_goto_finish_if_not( ps < pe );
			}

			switch( ps->type ) {
			case KW_PUBLIC:
			case KW_PROTECTED:
			case KW_PRIVATE:
				++ps;
				err_goto_finish_if_not( ps < pe );
				break;
			}

			err_goto_finish_if( (ps = parse_id(ps, pe, &(inhers[inhers_count]), 0))==0 );
			++inhers_count;
		} while( (ps < pe) && ps->type==',' );

		if( inhers_count ) {
			elem->v_class.inhers_count = inhers_count;
			elem->v_class.inhers = g_memdup(inhers, sizeof(TinyStr*)*inhers_count);

			inhers_count = 0;
		}
	}

	err_goto_finish_if_not( ps < pe );
	if( ps->type!=';' ) {
		err_goto_finish_if( ps->type!='{' );
		++ps;

		ps = parse_scope(env, ps, (pe - ps), elem, TRUE);
		err_goto_finish_if( ps==0 );

		if( ((ps+1) < pe) && (ps+1)->type!=';' ) {
			Block var_block = { block->parent, ps, (pe - ps), BLOCK_STYLE_LINE, block->scope };
			MLToken type_token = *ps;

			ps->type = TK_ID;
			ps->buf = elem->name->buf;
			ps->len = tiny_str_len(elem->name);
			ps->line = (ps+1)->line;

			cps_var(env, &var_block);
			
			*ps = type_token;
		}
	}

__cps_finish__:
	tiny_str_free(nskey);
	for( ; inhers_count>0; --inhers_count )
		tiny_str_free(inhers[inhers_count-1]);
	return TRUE;
}

