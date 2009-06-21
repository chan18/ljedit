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

typedef struct {
	gchar (*do_prev)(gpointer it);
	gchar (*do_next)(gpointer it);
} SearchIterEnv;

typedef struct _Searcher Searcher;

GList* cpp_spath_find(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith);
GList* cpp_spath_parse(const gchar* text, gboolean find_startswith);
void   cpp_spath_free(GList* spath);

typedef void (*CppMatched)(CppElem* elem, gpointer* tag);

void cpp_search( CppSTree* stree
	, GList* spath
	, CppMatched cb
	, gpointer cb_tag
	, CppFile* file
	, gint line );

#endif//PUSS_CPP_SEARCHER_H

