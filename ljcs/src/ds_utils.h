// ds_utils.h
// 

#ifndef LJCS_DS_UTILS_H
#define LJCS_DS_UTILS_H

#include "ds.h"

namespace cpp {

void scope_insert(Scope& scope, Element* elem);

inline void scope_insert_elems(Scope& scope, Elements& elems) {
	Elements::iterator it = elems.begin();
	Elements::iterator end = elems.end();
	for( ; it != end; ++it )
		scope_insert(scope, *it);
}

}// namespace cpp

#endif//LJCS_DS_UTILS_H

