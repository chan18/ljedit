// cps_class.c
// 

#include "cps_utils.h"

gboolean cps_class(ParseEnv* env, Block* block) {
	MLToken* ps;
	MLToken* pe;
	CppElem* elem = 0;
	TinyStr* nskey = 0;
	MLToken* name = 0;
	gint class_type = KD_UNK;

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

	if( ps->type==TK_ID )
		err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, &name))==0 );

	elem = cpp_elem_new();
	if( name ) {
		if( name->len==nskey->len ) {
			elem->name = nskey;
		} else {
			elem->name = tiny_str_new(name->buf, name->len);
			elem->v_class.nskey = nskey;
		}
		nskey = 0;
			
	} else {
		elem->name = tiny_str_new("@anonymous", 10);
	}

	elem->v_class.class_type = class_type==KW_STRUCT ? 's' : (class_type==KW_CLASS ? 'c' : (class_type==KW_UNION ? 'u' : '?'));
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

			err_goto_finish_if( (ps = parse_id(ps, pe, &nskey, 0))==0 );
			// TODO :
			// elem->v_class.inhers.push_back(nskey);
			{
				tiny_str_free(nskey);
				nskey = 0;
			}

		} while( (ps < pe) && ps->type==',' );
	}

	err_goto_finish_if_not( ps < pe );
	if( ps->type!=';' ) {
		err_goto_finish_if( ps->type!='{' );
		++ps;

		ps = parse_scope(env, ps, (pe - ps), elem, TRUE)
		err_goto_finish_if( ps==0 );
		g_assert( (ps < pe) && ps->type=='}' );

		if( ((ps+1) < pe) && (ps+1)->type!=';' ) {
			Block var_block = { block->parent, ps, (pe - ps), BLOCK_STYLE_LINE, block->scope };
			MLToken type_token = *ps;

			ps->type = TK_ID;
			ps->buf = elem->name->buf;
			ps->len = elem->name->len;
			ps->line = (ps+1)->line;

			cps_var(&var_block);
			
			*ps = type_token;
		}
	}

__cps_finish__:
	tiny_str_free(nskey);
	return TRUE;
}

