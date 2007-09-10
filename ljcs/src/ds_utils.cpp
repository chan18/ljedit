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

Scope& get_index_scope(File& file, Scope& scope, const std::string& nskey) {
	if( nskey.empty() )
		return scope;

	Scope* retval = &scope;
	std::string key;
	size_t ps = 0;
	size_t pe = ps;
	while( ps != nskey.npos ) {
		pe = nskey.find('.', ps);
		if( pe!=nskey.npos ) {
			key = nskey.substr(ps, pe-ps);
			++pe;
		} else {
			key = nskey.substr(ps);
			assert( !key.empty() );
		}
		ps = pe;
		
		cpp::IndexMap::iterator it = retval->imap.lower_bound(key);
		cpp::IndexMap::iterator end = retval->imap.upper_bound(key);
		for( ; it!=end; ++it ) {
			cpp::Element& elem = retval->imap.get(it);
			if( elem.type==ET_NCSCOPE )
			{
				NCScope* p = (NCScope*)(&elem);
				retval = &(p->scope);
				break;
			}
		}
		
		if( it==end ) {
			NCScope* p = Element::create<NCScope>(file, key, 0, 0);
			retval->__index_used_elems.push_back(p);
			retval->insert_index(p);
			retval = &(p->scope);
		}
	}
	return *retval;
}

inline void do_scope_insert_index(Scope& scope, Element* elem) {
	switch( elem->type ) {
	case ET_INCLUDE: {
			elem->file.includes.push_back( (Include*)elem );
		}
		break;
	case ET_VAR: {
			Var& r = (Var&)(*elem);
			get_index_scope(r.file, scope, r.nskey).insert_index(elem);
		}
		break;
	case ET_FUN: {
			Function& r = (Function&)(*elem);
			get_index_scope(r.file, scope, r.nskey).insert_index(elem);
		}
		break;
	case ET_CLASS: {
			if( !elem->name.empty() && elem->name[0]!='@' )
				scope.insert_index(elem);
		}
		break;
	case ET_MACRO:
	case ET_TYPEDEF: {
			scope.insert_index(elem);
		}
		break;
	case ET_ENUM: {
			if( !elem->name.empty() && elem->name[0]!='@' )
				scope.insert_index(elem);
			
			Enum* p = (Enum*)elem;
			Elements::iterator it = p->scope.elems.begin();
			Elements::iterator end = p->scope.elems.end();
			for( ; it != end; ++it )
				scope.insert_index(*it);
		}
		break;
	case ET_USING: {
			if( ((Using*)elem)->isns )
				scope.usings.push_back( (Using*)elem );
			else
				scope.insert_index(elem);
		}
		break;
	case ET_NAMESPACE: {
			Namespace* p = (Namespace*)elem;
			if( p->name.empty() || p->name[0]=='@' ) {
				// anonymous namespace need not make index
				if( &(p->scope) == &scope )
					break;

				// insert anonymous namespace elems to current scope
				assert( p->scope.imap.empty() );
				
				Elements::iterator it = p->scope.elems.begin();
				Elements::iterator end = p->scope.elems.end();
				for( ; it != end; ++it )
					do_scope_insert_index(scope, *it);

			} else {
				scope.insert_index(elem);
			}
		}
		break;
	}
}

void scope_insert(Scope& scope, Element* elem) {
	assert( elem != 0 );

	if( elem->type != ET_NCSCOPE )
		do_scope_insert_element(scope, elem);

	do_scope_insert_index(scope, elem);
}

};// namespace cpp

