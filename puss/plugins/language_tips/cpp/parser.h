// parser.h
// 
#ifndef PUSS_CPP_PARSER_H
#define PUSS_CPP_PARSER_H

#include "ds.h"

typedef struct {
	gboolean		enable_macro_replace;
	gpointer		keywords_table;

	GStaticRWLock	include_paths_lock;
	IncludePaths*	include_paths;

	GStaticRWLock	files_lock;
	GHashTable*		files;
} CppParser;

void cpp_parser_init(CppParser* parser, gboolean enable_macro_replace);
void cpp_parser_final(CppParser* parser);

void cpp_parser_include_paths_set(CppParser* parser, GList* paths);
IncludePaths* cpp_parser_include_paths_ref(CppParser* parser);
void cpp_parser_include_paths_unref(IncludePaths* paths);

CppFile* cpp_parser_find_parsed(CppParser* parser, const gchar* filekey);
CppFile* cpp_parser_parse(CppParser* parser, const gchar* filekey);

#endif//PUSS_CPP_PARSER_H

