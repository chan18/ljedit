// searcher.h
// 

#ifndef PUSS_CPP_SEARCHER_H
#define PUSS_CPP_SEARCHER_H

#include "parser.h"

typedef struct _SNode SNode;

typedef struct {
	GStaticRWLock	lock;
	SNode*			root;
	CppFile			keywords_file;
} CppSTree;

void stree_init(CppSTree* self);
void stree_final(CppSTree* self);
void stree_insert(CppSTree* self, CppFile* file);
void stree_remove(CppSTree* self, CppFile* file);

typedef struct {
	CppTextIter	do_prev;
	CppTextIter	do_next;
} SearchIterEnv;

GList*   spath_find(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith);
GList*   spath_parse(const gchar* text, gboolean find_startswith);
void     spath_free(GList* spath);
gboolean spath_equal(GList* a, GList* b);

void searcher_search( CppSTree* stree
	, GList* spath
	, gboolean (*cb)(CppElem* elem, gpointer tag)
	, gpointer cb_tag
	, CppFile* file
	, gint line
	, gint limit_num
	, gint limit_time );

#endif//PUSS_CPP_SEARCHER_H

