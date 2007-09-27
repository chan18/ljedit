// ds_utils.cpp
// 

#include "crt_debug.h"

#include "ds_utils.h"

namespace cpp {

inline void do_scope_insert_element(Scope& scope, Element* elem) {
	Elements& elems = scope.elems;
	Elements::iterator it = elems.end();
	Elements::iterator ep = elems.begin();
	while( it != ep ) {
		if( elem->sline >= (*(it - 1))->sline )
			break;
		--it;
	}
	elems.insert(it, elem);
}

void scope_insert(Scope& scope, Element* elem) {
	assert( elem != 0 );

	if( elem->type != ET_NCSCOPE )
		do_scope_insert_element(scope, elem);
}

};// namespace cpp

