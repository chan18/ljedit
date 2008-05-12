// ds.c
// 

#include "ds.h"


TinyStr* tiny_str_new(gchar* buf, gsize len) {
	TinyStr* res = (TinyStr*)g_new(gchar, (sizeof(TinyStr) + len));
	res->len = len;
	memcpy(res->buf, buf, len);
	res->buf[len] = '\0';
	return res;
}

gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b) {
	return ( a->len==b->len )
		? (memcmp(a->buf, b->buf, a->len)==0)
		: FALSE;
}

