// searcher.h
// 

#ifndef LJCS_SEARCHER_H
#define LJCS_SEARCHER_H

#include "ds.h"

class IMatched {
public:
	IMatched() {}
	virtual ~IMatched() {}
public:
	virtual void on_matched(cpp::Element& elem) = 0;
};

class MatchedPrint : public IMatched {
public:
	virtual void on_matched(cpp::Element& elem) {
		std::cout << elem.decl << std::endl;
	}
};

class MatchedSet : public IMatched {
public:
	cpp::ElementSet elems;

	virtual void on_matched(cpp::Element& elem) {
		elems.insert(&elem);
	}
};

void search( const std::string& key
	, IMatched& cb
	, cpp::File& file
	, size_t line = 0);

void search_keys(const StrVector& keys, IMatched& cb, cpp::File& file, size_t line=0);

#endif//LJCS_SEARCHER_H

