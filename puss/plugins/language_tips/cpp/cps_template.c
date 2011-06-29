// cps_template.c
// 

#include "cps_utils.h"

MLToken* parse_template_arg_value(MLToken* ps, MLToken* pe) {
	MLToken* retval = 0;
	while( ps && ps < pe ) {
		if( ps->type==',' ) {
			++ps;
			retval = ps;
			break;

		} else if( ps->type=='>' ) {
			retval = ps;
			break;
		}

		++ps;
		if( ps < pe ) {
			switch( ps->type ) {
			case '(':	ps = skip_pair_round_brackets(ps, pe);	break;
			case '<':	ps = skip_pair_angle_bracket(ps, pe);	break;
			case '[':	ps = skip_pair_brace_bracket(ps, pe);	break;
			}
		}
	}

	return retval;
}

#define TEMPLATE_ARGS_MAX 128

gboolean cps_template(ParseEnv* env, Block* block) {
	// Template* tmpl;
	gint i;
	gint targc = -1;
	TemplateArg targv[128];
	TemplateArg* targ;
	gint dt;

	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;

	while( (ps < pe) && ps->type!=KW_TEMPLATE )
		++ps;

	++ps;
	err_goto_finish_if_not( (ps < pe) && ps->type=='<' );

	++ps;
	while( ps < pe ) {
		if( ps->type=='>' )
			break;

		err_goto_finish_if_not( (targc + 1) < TEMPLATE_ARGS_MAX );
		++targc;
		targ = targv + targc;
		memset(targ, 0, sizeof(TemplateArg));

		if( ps->type==KW_TYPENAME || ps->type==KW_CLASS ) {
			targ->type = tiny_str_new(ps->buf, ps->len);
			++ps;
			if( (ps < pe) && ps->type==TK_ID ) {
				targ->name = tiny_str_new(ps->buf, ps->len);
				++ps;
			}
			
		} else {
			dt = KD_UNK;
			err_goto_finish_if( (ps = parse_datatype(ps, pe, &(targ->type), &dt))==0 );
			err_goto_finish_if( (ps = parse_ptr_ref(ps, pe, &dt))==0 );
			err_goto_finish_if_not( ps < pe );

			if( ps->type=='(' ) {
				// try parse function pointer
				err_goto_finish_if( (ps = skip_pair_round_brackets(ps+1, pe))==0 );
				err_goto_finish_if( ps->type!='(' );
				err_goto_finish_if( (ps = skip_pair_round_brackets(ps+1, pe))==0 );
				while( (ps < pe) && ps->type!=',' && ps->type!='>' )
					++ps;

				// TODO : <not finished>, now ignore this
				tiny_str_free(targ->type);
				targ->type = 0;
				--targc;

			} else if( ps->type==TK_ID ) {
				targ->name = tiny_str_new(ps->buf, ps->len);
				++ps;
			}
		}

		err_goto_finish_if_not( ps < pe );
		if( ps->type=='=' )
			err_goto_finish_if( (ps = parse_template_arg_value(ps+1, pe))==0 );

		err_goto_finish_if_not( ps < pe );
		if( ps->type==',' )
			++ps;
		else
			break;
	}

	err_goto_finish_if_not( (ps < pe) && ps->type=='>' );
	++ps;

	// TODO : <not finished>
	// /usr/include/c++/4.0/bits/basic_string.tcc:88
	// multi-template
	// 
	if( ps->type==KW_TEMPLATE ) {
		Block subblock = { block->parent, ps, (pe - ps), block->style };
		for( i=0; i<targc; ++i ) {
			tiny_str_free(targv[i].type);
			tiny_str_free(targv[i].name);
			tiny_str_free(targv[i].value);
		}
		return cps_template(env, &subblock);
	}

	parse_scope(env, ps, (pe - ps), block->parent, TRUE);
	return TRUE;

__cps_finish__:
	for( i=0; i<targc; ++i ) {
		tiny_str_free(targv[i].type);
		tiny_str_free(targv[i].name);
		tiny_str_free(targv[i].value);
	}

	return TRUE;
}

gboolean cps_extern_template(ParseEnv* env, Block* block) {
	// ignore

	return TRUE;
}

