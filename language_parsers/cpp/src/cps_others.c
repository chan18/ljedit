// cps_others.c
// 

#include "cps_utils.h"

#include <assert.h>


gboolean cps_block(Block* block, CppElem* parent) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	while( (ps < pe) && (ps->type != '{') )
		++ps;
	++ps;

	if( ps < pe )
		parse_scope(block->env, ps, (pe - ps), parent, FALSE);

	return TRUE;
}

gboolean cps_extern_scope(Block* block, CppElem* parent) {
	return cps_block(block, parent);
}

gboolean cps_impl_block(Block* block, CppElem* parent) {
	MLToken* ps = block->tokens;
	MLToken* pe = ps + block->count;
	while( (ps < pe) && (ps->type != '{') )
		++ps;
	++ps;

	if( ps < pe ) {
		block->tokens = ps;
		block->count = pe - ps;
		parse_impl_scope(block, parent);
	}

	return TRUE;
}

gboolean cps_label(Block* block, CppElem* parent) {
	assert( block->count > 0 );

	switch( block->tokens->type ) {
	case KW_PUBLIC:		block->scope = BLOCK_SCOPE_PUBLIC;		break;
	case KW_PROTECTED:	block->scope = BLOCK_SCOPE_PROTECTED;	break;
	case KW_PRIVATE:	block->scope = BLOCK_SCOPE_PRIVATE;		break;
	}

	return TRUE;
}

gboolean cps_skip_block(Block* block, CppElem* parent) {
	return TRUE;
}

