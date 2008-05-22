// cps_class.c
// 

#include "cps_utils.h"

gboolean cps_class(Block* block, GList* scope) {
	CppElem* elem = 0;
	TinyStr* nskey = 0;
	MLToken* name = 0;
	gint class_type = KD_UNK;
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
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

	elem = g_new0(CppElem, 1);
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

	//scope_insert(scope, p);

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
				g_free(nskey);
				nskey = 0;
			}

		} while( (ps < pe) && ps->type==',' );
	}

	err_goto_finish_if_not( ps < pe );
	if( ps->type!=';' ) {
		err_goto_finish_if( ps->type!='{' );
		++ps;

		// TODO : 
		//parse_scope(lexer, p->scope, p);

		err_goto_finish_if_not( (ps < pe) && ps->type=='}' );
		++ps;

		//TODO :
		//parse_var_define(ps, pe, scope, ..., class_type);
	}

__cps_finish__:
	g_free(nskey);
	return TRUE;
}

