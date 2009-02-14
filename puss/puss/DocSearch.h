// DocSearch.h
// 

#ifndef PUSS_INC_DOC_SEARCH_H
#define PUSS_INC_DOC_SEARCH_H

#include "Puss.h"

gboolean	puss_find_and_locate_text( GtkTextView* view
				, const gchar* text
				, gboolean is_forward
				, gboolean skip_current
				, gboolean mark_current
				, gboolean mark_all
				, gboolean is_continue
				, int search_flags );

#endif//PUSS_INC_DOC_SEARCH_H

