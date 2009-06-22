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

void stree_init(CppSTree* self);
void stree_final(CppSTree* self);
void stree_insert(CppSTree* self, CppFile* file);
void stree_remove(CppSTree* self, CppFile* file);

typedef struct {
	gchar (*do_prev)(gpointer it);
	gchar (*do_next)(gpointer it);
} SearchIterEnv;

GList* spath_find(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith);
GList* spath_parse(const gchar* text, gboolean find_startswith);
void   spath_free(GList* spath);

typedef void (*CppMatched)(CppElem* elem, gpointer* tag);

void searcher_search( CppSTree* stree
	, GList* spath
	, CppMatched cb
	, gpointer cb_tag
	, CppFile* file
	, gint line );

#endif//PUSS_CPP_SEARCHER_H

