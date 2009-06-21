// Utils.c
// 

#include "LanguageTips.h"

static gchar text_buffer_iter_do_prev(GtkTextIter* pos) {
	return gtk_text_iter_backward_char(pos)
		? (gchar)gtk_text_iter_get_char(pos)
		: '\0';
}

static gchar text_buffer_iter_do_next(GtkTextIter* pos) {
	return gtk_text_iter_forward_char(pos)
		? (gchar)gtk_text_iter_get_char(pos)
		: '\0';
}

/*
gchar* text_buffer_find_key( CppFile* file
	, GtkTextIter* it
    , GtkTextIter* end
	, gboolean find_startswith )
{
	SearchIterEnv env = { text_buffer_iter_do_prev, text_buffer_iter_do_next };
	gchar* key;

	key = cpp_find_key(&env, it, end, find_startswith);
	return key;
}
*/

