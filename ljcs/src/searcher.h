// searcher.h
// 

#ifndef LJCS_SEARCHER_H
#define LJCS_SEARCHER_H

#include "ds.h"
#include "parser.h"
#include "index.h"

#include <iostream>


class IMatched {
public:
	IMatched(IParserEnviron& env) : env_(env) {}
	virtual ~IMatched() {}
public:
	virtual void on_matched(cpp::Element& elem) = 0;
protected:
	IParserEnviron& env_;
};

class MatchedPrint : public IMatched {
public:
	virtual void on_matched(cpp::Element& elem) {
		std::cout << elem.decl << std::endl;
	}
};

class MatchedSet : public IMatched {
public:
	MatchedSet(IParserEnviron& env) : IMatched(env) {}
	~MatchedSet() { clear(); }

	cpp::ElementSet elems_;

	virtual void on_matched(cpp::Element& elem) {
		cpp::ElementSet::iterator it = elems_.find(&elem);
		if( it==elems_.end() ) {
			env_.pe_file_incref(&(elem.file));
			elems_.insert(&elem);
		}
	}

	cpp::ElementSet& elems()          { return elems_; }

	size_t size() const               { return elems_.size(); }

	cpp::ElementSet::iterator begin() { return elems_.begin(); }

	cpp::ElementSet::iterator end()   { return elems_.end(); }

	void clear() {
		env_.file_decref_all_elems(elems_);
		elems_.clear();
	}
};

void search( const std::string& key
	, IMatched& cb
	, cpp::STree& stree
	, cpp::File* file=0
	, size_t line=0 );

void search_keys( const StrVector& keys
	, IMatched& cb
	, cpp::STree& stree
	, cpp::File* file=0
	, size_t line=0 );

#endif//LJCS_SEARCHER_H

