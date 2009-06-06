// keywords.h
// 
#ifndef PUSS_CPP_KEYWORDS_H
#define PUSS_CPP_KEYWORDS_H

#include "token.h"

gpointer cpp_keywords_table_new();
void cpp_keywords_table_free(gpointer keywords_table);

void cpp_keywords_check(MLToken* token, gpointer keywords_table);

#endif//PUSS_CPP_KEYWORDS_H

