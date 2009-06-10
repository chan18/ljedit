// cps_others.c
// 

#include "cps_utils.h"

#include <assert.h>


gboolean cps_block(ParseEnv* env, Block* block) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	while( (ps < pe) && (ps->type != '{') )
		++ps;
	++ps;

	if( ps < pe )
		parse_scope(env, ps, (pe - ps), block->parent, FALSE);

	return TRUE;
}

gboolean cps_extern_scope(ParseEnv* env, Block* block) {
	return cps_block(env, block);
}

gboolean cps_impl_block(ParseEnv* env, Block* block) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	while( (ps < pe) && (ps->type != '{') )
		++ps;
	++ps;

	if( ps < pe ) {
		block->tokens = ps;
		block->count = pe - ps;
		parse_impl_scope(block);
	}

	return TRUE;
}

gboolean cps_label(ParseEnv* env, Block* block) {
	assert( block->count > 0 );

	switch( block->tokens->type ) {
	case KW_PUBLIC:		block->scope = BLOCK_SCOPE_PUBLIC;		break;
	case KW_PROTECTED:	block->scope = BLOCK_SCOPE_PROTECTED;	break;
	case KW_PRIVATE:	block->scope = BLOCK_SCOPE_PRIVATE;		break;
	}

	return TRUE;
}

gboolean cps_skip_block(ParseEnv* env, Block* block) {
	return TRUE;
}

