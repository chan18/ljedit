// index.h
// 
#ifndef LJCS_INDEX_H
#define LJCS_INDEX_H

#include "ds.h"

namespace cpp {

class SNode;
typedef std::map<std::string, SNode>	SMap;

class SNode {
public:
	SNode() : sub_(0) {}

	~SNode() {
		if( sub_!=0 ) {
			delete sub_;
			sub_ = 0;
		}
	}

	ElementSet&	elems()  { return elems_; }

	bool has_sub() const { return sub_!=0; }

	bool empty() const   { return elems_.empty() && !has_sub(); }

	SMap& sub() {
		if( sub_==0 ) {
			sub_ = new SMap;
			if( sub_==0 )
				throw std::runtime_error("no enough memeory");
		}
		return *sub_;
	}

	SNode* find_sub_index(Element& elem) {
		if( has_sub() ) {
			SMap::iterator it = sub_->find(elem.name);
			if( it!=sub_->end() ) {
				return &it->second;
			}
		}

		return 0;
	}

	SNode& insert_sub_index(Element& elem) {
		SNode& node = sub()[elem.name];
		node.elems_.insert(&elem);
		return node;
	}


private:
	ElementSet	elems_;
	SMap*		sub_;
};

class STree {
public:
	SNode& root() { return root_; }

	void add(File& file);
	void remove(File& file);

	void add(Scope& scope);

private:
	SNode& get_index_scope_node(File& file, SNode& parent, const std::string& nskey);
	void add(SNode& node, Element& elem);
	void remove(SNode& node, File& file);

private:
	SNode	root_;
	FileSet	files_;
};

}//namespace cpp

#endif//LJCS_INDEX_H
