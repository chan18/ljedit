// searcher.h
// 

#ifndef PUSS_CPP_SEARCHER_H
#define PUSS_CPP_SEARCHER_H

#include "parser.h"

typedef struct _SNode SNode;

typedef struct {
	GStaticRWLock	lock;
	SNode*			root;
} CppSTree;

void cpp_stree_init(CppSTree* self);
void cpp_stree_final(CppSTree* self);
void cpp_stree_insert(CppSTree* self, CppFile* file);
void cpp_stree_remove(CppSTree* self, CppFile* file);


typedef void (*CppMatched)(CppElem* elem, gpointer* tag);

void cpp_search( CppSTree* stree
	, const gchar* key
	, CppMatched cb
	, gpointer cb_tag
	, CppFile* file
	, gint line );

#endif//PUSS_CPP_SEARCHER_H

