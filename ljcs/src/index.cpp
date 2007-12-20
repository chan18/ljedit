// searcher.cpp
// 
#include "crt_debug.h"

#include "index.h"
#include "parser.h"

namespace cpp {

void STree::clear() {
	root_.clear();

	FileSet::iterator it = files_.begin();
	FileSet::iterator end = files_.end();
	for( ; it!=end; ++it )
		env_->pe_file_decref(*it);

	files_.clear();
}

void STree::add(File& file) {
	if( files_.find(&file)!=files_.end() )
		return;

	files_.insert(&file);
	env_->pe_file_incref(&file);
	add(file.scope);
}

/*
void STree::remove(File& file) {
	FileSet::iterator it = files_.find(&file);
	if( it==files_.end() )
		return;

	files_.erase(it);

	remove(root_, file);
}
*/

void STree::add(Scope& scope) {
	Elements::iterator it = scope.elems.begin();
	Elements::iterator end = scope.elems.end();
	for( ; it != end; ++it )
		add(root_, **it);
}

SNode& STree::get_index_scope_node(File& file, SNode& parent, const std::string& nskey) {
	if( nskey.empty() )
		return parent;

	SNode* retval = &parent;
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
		
		retval = &(retval->sub()[key]);
	}
	return *retval;
}

void STree::add(SNode& parent, Element& elem) {
	switch( elem.type ) {
	case ET_INCLUDE:
		if( env_ != 0 ) {
			File* file = env_->pe_find_parsed(((Include&)elem).include_file);
			if( file != 0 ) {
				add( *file );
				env_->pe_file_decref(file);
			}
		}
		break;
	case ET_VAR: {
			Var& r = (Var&)elem;
			get_index_scope_node(r.file, parent, r.nskey).insert_sub_index(elem);
		}
		break;
	case ET_FUN: {
			Function& r = (Function&)elem;
			get_index_scope_node(r.file, parent, r.nskey).insert_sub_index(elem);
		}
		break;
	case ET_CLASS: {
			Class& r = (Class&)elem;
			if( !r.name.empty() && r.name[0]!='@' ) {
				SNode& snode = parent.insert_sub_index(elem);
				
				Elements::iterator it = r.scope.elems.begin();
				Elements::iterator end = r.scope.elems.end();
				for( ; it != end; ++it )
					add(snode, **it);
			}
		}
		break;
	case ET_MACRO:
	case ET_TYPEDEF:
	case ET_ENUMITEM: {
			parent.insert_sub_index(elem);
		}
		break;
	case ET_ENUM: {
			SNode* snode = 0;
			if( !elem.name.empty() && elem.name[0]!='@' )
				snode = &parent.insert_sub_index(elem);
			
			Enum& r = (Enum&)elem;
			Elements::iterator it = r.scope.elems.begin();
			Elements::iterator end = r.scope.elems.end();
			for( ; it != end; ++it ) {
				parent.insert_sub_index(**it);
				if( snode!=0 )
					add(*snode, **it);
			}
		}
		break;
	case ET_USING: {
			if( ((Using&)elem).isns )
				; // scope.usings.push_back( (Using*)elem );
			else
				parent.insert_sub_index(elem);
		}
		break;
	case ET_NAMESPACE: {
			Namespace& r = (Namespace&)elem;

			// insert anonymous namespace elems to current scope
			SNode* snode = &parent;

			// anonymous namespace need not make index
			if( !r.name.empty() && r.name[0]!='@' )
				snode = &parent.insert_sub_index(elem);

			Elements::iterator it = r.scope.elems.begin();
			Elements::iterator end = r.scope.elems.end();
			for( ; it != end; ++it )
				add(*snode, **it);
		}
		break;
	}
}

/*
void STree::remove(SNode& node, File& file) {
	{
		SMap& smap = node.sub();
		SMap::iterator it = smap.begin();
		SMap::iterator end = smap.end();
		while( it!=end ) {
			remove(it->second, file);
			if( it->second.empty() )
				smap.erase(it++);
			else
				++it;
		}
	}

	{
		ElementSet& elems = node.elems();
		ElementSet::iterator it = elems.begin();
		ElementSet::iterator end = elems.end();
		while( it!=end ) {
			if( &((*it)->file)==&file )
				elems.erase(it++);
			else
				++it;
		}
	}
}
*/

}//namespace cpp

