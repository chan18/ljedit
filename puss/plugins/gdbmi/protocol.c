// protocol.c
// 

#include "protocol.h"

#include <memory.h>
#include <string.h>

static void mi_value_free(MIValue* v) {
	if( !v )
		return;

	switch( v->type ) {
	case 'c':
		g_free(v->v_const);
		break;
	case 't':
		g_hash_table_destroy(v->v_tuple);
		break;
	case 'l':
		g_list_foreach(v->v_list, mi_value_free, 0);
		break;
	}
	g_free(v);
}

static void mi_record_free(MIRecord* v) {
	if( !v )
		return;

	switch( v->type ) {
	case '*':
	case '+':
	case '=':
		{
			MIAsyncRecord* p = (MIAsyncRecord*)v;
			g_free( p->async_class );
			if( p->results )
				g_hash_table_destroy( p->results );
		}
		break;
	case '~':
	case '@':
	case '&':
		{
			MIStreamRecord* p = (MIStreamRecord*)v;
			g_free( p->str );
		}
		break;
	case '^':
		{
			MIResultRecord* p = (MIResultRecord*)v;
			g_free( p->result_class );
			if( p->results )
				g_hash_table_destroy( p->results );
		}
		break;
	}
	g_free(v);
}

#define MI_CURRENT		(self->buf[self->pos])
#define MI_CURRENT_PTR	(self->buf + self->pos)
#define MI_HAS_NEXT		(self->pos < self->len)

static gboolean mi_parse_token(MIParser* self, gchar** token) {
	gchar* vbuf;
	gsize  vlen;

	*token = 0;
	if( !g_ascii_isdigit(MI_CURRENT) )
		return FALSE;

	vbuf = MI_CURRENT_PTR;
	for( ++self->pos; MI_HAS_NEXT; ++self->pos )
		if( !g_ascii_isdigit(MI_CURRENT) )
			break;
	vlen = (self->buf + self->pos) - vbuf;

	*token = g_strndup(vbuf, vlen);
	return TRUE;
}

static gboolean mi_parse_string(MIParser* self, MIStr* str) {
	gchar ch;
	if( !g_ascii_isalpha(MI_CURRENT) )
		return FALSE;

	str->buf = MI_CURRENT_PTR;
	while( MI_HAS_NEXT ) {
		++self->pos;
		ch = MI_CURRENT;
		if( g_ascii_isalnum(ch) || ch=='-' || ch=='_' )
			continue;
		break;
	}
	str->len = (gsize)(MI_CURRENT_PTR - str->buf);
	return TRUE;
}

#define mi_is_CR_LF(self) (MI_CURRENT=='\r' || MI_CURRENT=='\n')

static gboolean mi_parse_CR_LF(MIParser* self) {
	if( MI_CURRENT=='\r' ) {
		++self->pos;
		if( MI_CURRENT=='\n' )
			++self->pos;
		return TRUE;
	}

	if( MI_CURRENT=='\n' ) {
		++self->pos;
		return TRUE;
	}

	return FALSE;
}

#define mi_is_async_record(self)       (MI_CURRENT=='*' || MI_CURRENT=='+' || MI_CURRENT=='=')
#define mi_is_stream_record(self)      (MI_CURRENT=='~' || MI_CURRENT=='@' || MI_CURRENT=='&')
#define mi_is_out_of_band_record(self) (mi_is_async_record(self) || mi_is_stream_record(self))
#define mi_is_result_record(self)      (MI_CURRENT=='^')
#define mi_is_value_sign(self)         (MI_CURRENT=='"' || MI_CURRENT=='{' || MI_CURRENT=='[')

static gboolean mi_parse_end_sign(MIParser* self) {
	if( self->len - self->pos < 5 )
		return FALSE;

	if( memcmp(MI_CURRENT_PTR, "(gdb)", 5)!=0 )
		return FALSE;

	self->pos += 5;
	while( MI_CURRENT==' ' )
		++self->pos;

	return mi_parse_CR_LF(self);
}

static gboolean mi_parse_c_string(MIParser* self, gchar** out) {
	gchar* vbuf;
	gsize  vlen;

	if( MI_CURRENT!='"' ) {
		*out = 0;
		return FALSE;	// Bad const
	}

	vbuf = MI_CURRENT_PTR + 1;
	while( MI_HAS_NEXT ) {
		++self->pos;
		if( MI_CURRENT=='"' ) {
			++self->pos;
			break;

		} else if( MI_CURRENT=='\\' ) {
			++self->pos;
		}
	}
	vlen = (gsize)( MI_CURRENT_PTR - 1 - vbuf );

	*out = g_strndup(vbuf, vlen);
	return TRUE;
}
static gboolean mi_parse_value(MIParser* self, MIValue* value);

static gboolean mi_parse_values(MIParser* self, GList** out) {
	MIValue* v;

	*out = 0;
	
	while( MI_HAS_NEXT ) {
		if( MI_CURRENT==']' )
			return TRUE;

		if( MI_CURRENT==',' )
			++self->pos;

		v = g_new0(MIValue, 1);
		*out = g_list_append(*out, v);

		if( !mi_parse_value(self, v) )
			goto __error__;
	}

__error__:
	g_list_foreach(*out, mi_value_free, 0);
	g_list_free(*out);
	*out = 0;
	return FALSE;
}

static gboolean mi_parse_results(MIParser* self, GHashTable** out) {
	MIStr name;
	MIValue* v;

	if( MI_CURRENT==']' || MI_CURRENT=='}' ) {
		*out = 0;
		return TRUE;
	}

	*out = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, mi_value_free);
	while( MI_HAS_NEXT ) {
		if( !mi_parse_string(self, &name) )
			goto __error__;

		if( MI_CURRENT!='=' )
			goto __error__;		//Not find "=" after name, when parse result
		++self->pos;

		v = g_new0(MIValue, 1);
		g_hash_table_insert(*out, g_strndup(name.buf, name.len), v);

		if( !mi_parse_value(self, v) )
			goto __error__;

		if( MI_CURRENT!=',' )
			return TRUE;
		++self->pos;
	}

__error__:
	g_hash_table_destroy(*out);
	*out = 0;
	return FALSE;
}

static gboolean mi_parse_const(MIParser* self, MIValue* value) {
	value->type = 'c';
	return mi_parse_c_string(self, &(value->v_const));
}

static gboolean mi_parse_tuple(MIParser* self, MIValue* value) {
	if( MI_CURRENT!='{' )
		return FALSE;

	++self->pos;
	value->type = 't';

	if( MI_CURRENT=='}' ) {
		value->v_tuple = 0;

	} else {
		if( !mi_parse_results(self, &(value->v_tuple)) )
			return FALSE;

		if( MI_CURRENT!='}' ) {
			mi_value_free(value);
			return FALSE;	// Bad tuple
		}
	}

	++self->pos;
	return TRUE;
}

static gboolean mi_parse_list(MIParser* self, MIValue* value) {
	if( MI_CURRENT!='[' )
		return FALSE;
	++self->pos;

	if( MI_CURRENT==']' ) {
		value->v_list = 0;

	} else {
		if( mi_is_value_sign(self) ) {
			value->type = 'l';
			if( !mi_parse_values(self, &(value->v_list)) )
				return FALSE;
		} else {
			value->type = 't';
			if( !mi_parse_results(self, &(value->v_tuple)) )
				return FALSE;
		}

		if( MI_CURRENT!=']' ) {
			mi_value_free(value);
			return FALSE;	// Bad list
		}
	}

	++self->pos;
	return TRUE;
}

static gboolean mi_parse_value(MIParser* self, MIValue* value) {
	switch( MI_CURRENT ) {
	case '\"':
		return mi_parse_const(self, value);
	case '{':
		return mi_parse_tuple(self, value);
	case '[':
		return mi_parse_list(self, value);
	}

	return FALSE;
}

static gboolean mi_parse_async_output(MIParser* self, MIAsyncRecord* record) {
	// async-output -> async-class ( "," result )* nl

	MIStr type;
	GHashTable* results;

	if( !mi_parse_string(self, &type) )
		return FALSE;

	results = 0;
	if( !mi_is_CR_LF(self) ) {
		if( MI_CURRENT!=',' )
			return FALSE;	// Not Find ',' when parse async-output
		++self->pos;

		if( !mi_parse_results(self, &results) )
			return FALSE;
	}

	if( !mi_parse_CR_LF(self) ) {
		if( results )
			g_hash_table_destroy(results);
		return FALSE;
	}

	record->async_class = g_strndup(type.buf, type.len);
	record->results = results;
	return TRUE;
}

static gboolean mi_parse_async_record(MIParser* self, gulong token, MIAsyncRecord* record) {
	gboolean res = FALSE;

	record->token = token;
	res = mi_parse_async_output(self, record);

	if( !res )
		return FALSE;	// Bad/NotFind async-record sign

	return TRUE;
}

static gboolean mi_parse_stream_record(MIParser* self, MIStreamRecord* record) {
	if( !mi_parse_c_string(self, &(record->str)) )
		return FALSE;

	mi_parse_CR_LF(self);	// need
	return TRUE;
}

static gboolean mi_parse_out_of_band_records(MIParser* self, MIParseResult* out) {
	gboolean res = FALSE;
	gulong token = 0;
	gint last_pos;

	while( MI_HAS_NEXT ) {
		MIRecord* record = 0;

		last_pos = self->pos;
		mi_parse_token(self, &token);
		
		if( mi_is_async_record(self) ) {
			MIAsyncRecord* async_record = g_new0(MIAsyncRecord, 1);
			record = (MIRecord*)async_record;
			record->type = MI_CURRENT;
			++self->pos;

			if( !mi_parse_async_record(self, token, async_record) ) {
				mi_record_free(record);
				return FALSE;
			}

		} else if( mi_is_stream_record(self) ) {
			MIStreamRecord* stream_record = g_new0(MIStreamRecord, 1);
			record = (MIRecord*)stream_record;
			record->type = MI_CURRENT;
			++self->pos;

			if( !mi_parse_stream_record(self, stream_record) ) {
				mi_record_free(record);
				return FALSE;
			}

		} else {
			self->pos = last_pos;
			break;
		}

		if( record )
			out->out_of_bands = g_list_append(out->out_of_bands, record);
	}

	return TRUE;
}

static gboolean mi_parse_result_record(MIParser* self, MIParseResult* out) {
	// result-record -> [ token ] "^" result-class ( "," result )* nl

	gulong token = 0;
	MIResultRecord* result = 0;
	MIStr result_class;

	mi_parse_token(self, &token);

	if( !mi_is_result_record(self) )
		return TRUE;	// Allowed Empty Result

	++self->pos;

	if( !mi_parse_string(self, &result_class) )
		return FALSE;

	result = g_new0(MIResultRecord, 1);
	((MIRecord*)result)->type = '^';

	if( mi_is_CR_LF(self) ) {
		result->token = token;
		result->result_class = g_strndup(result_class.buf, result_class.len);

	} else {
		if( MI_CURRENT!=',' ) { 
			mi_record_free( (MIRecord*)result );
			return FALSE;
		}

		++self->pos;

		if( !mi_parse_results(self, &(result->results)) ) {
			mi_record_free( (MIRecord*)result );
			return FALSE;
		}	
	}

	mi_parse_CR_LF(self);

	out->result_record = result;
	return TRUE;
}

MIParseResult* mi_result_parse(const gchar* buf, gint len, gsize* bytes_read) {
	MIParser parser;
	MIParseResult* res;

	parser.buf = (gchar*)buf;
	parser.len = len ? len : (gsize)strlen(buf);
	parser.pos = 0;

	// output -> ( out-of-band-record )* [ result-record ] "(gdb)" nl

	res = g_new0(MIParseResult, 1);

	if( !mi_parse_out_of_band_records(&parser, res)
		|| !mi_parse_result_record(&parser, res)
		|| !mi_parse_end_sign(&parser) )
	{
		mi_result_free(res);
		res = 0;
		
	} else if( bytes_read ) {
		*bytes_read = parser.pos;
	}

	return res;
}

void mi_result_free(MIParseResult* v) {
	if( !v )
		return;

	g_list_foreach(v->out_of_bands, mi_record_free, 0);
	g_list_free(v->out_of_bands);

	mi_record_free((MIRecord*)(v->result_record));

	g_free(v);
}

