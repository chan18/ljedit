// parser.h
// 
#ifndef PUSS_CPP_PARSER_H
#define PUSS_CPP_PARSER_H

#include "ds.h"

typedef struct {
	gboolean		enable_macro_replace;
	gpointer		keywords_table;

	GList*			include_paths;

	GHashTable*		parsing_files;
	GHashTable*		parsed_files;
} CppParser;

void cpp_parser_init(CppParser* parser, gboolean enable_macro_replace);
void cpp_parser_final(CppParser* parser);

gchar* cpp_parser_filename_to_filekey(const gchar* filename, glong namelen);
CppFile* cpp_parser_parse(CppParser* parser, const gchar* filekey, glong keylen);

#endif//PUSS_CPP_PARSER_H

