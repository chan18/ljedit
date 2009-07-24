// protocol.h
// 

#ifndef PUSS_MI_PROTOCOL_H
#define PUSS_MI_PROTOCOL_H

/*
<<<GDB/MI output format>>>
 
output ->
	( out-of-band-record )* [ result-record ] "(gdb)" nl
result-record ->
	[ token ] "^" result-class ( "," result )* nl
out-of-band-record ->
	async-record | stream-record
async-record ->
	exec-async-output | status-async-output | notify-async-output
exec-async-output ->
	[ token ] "*" async-output
status-async-output ->
	[ token ] "+" async-output
notify-async-output ->
	[ token ] "=" async-output
async-output ->
	async-class ( "," result )* nl
result-class ->
	"done" | "running" | "connected" | "error" | "exit"
async-class ->
	"stopped" | others (where others will be added depending on the needs—this
	is still in development).
result ->
	variable "=" value
variable ->
	string
value -> const | tuple | list
const -> c-string
tuple -> "{}" | "{" result ( "," result )* "}"
list -> "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
stream-record ->
	console-stream-output | target-stream-output | log-stream-output
console-stream-output ->
	"~" c-string
target-stream-output ->
	"@" c-string
log-stream-output ->
	"&" c-string
nl -> CR | CR-LF
token -> any sequence of digits
*/

#include <glib.h>

typedef struct {
	gchar*	buf;
	gsize	len;
	gsize	pos;
} MIParser;

typedef struct {
	gchar*	buf;
	gsize	len;
} MIStr;

typedef struct {
	gchar	type;
	union {
		gchar*		v_const;
		GHashTable*	v_tuple;
		GList*		v_list;
	};
} MIValue;

typedef struct {
	gchar	type;	// async_record(*+=) | stream_record(~@&)
} MIRecord;

typedef struct {
	MIRecord	parent;

	gchar*		token;
	gchar*		async_class;
	GHashTable*	results;		// key(gchar*) : value(MIValue)
} MIAsyncRecord;

typedef struct {
	MIRecord	parent;

	gchar*		str;
} MIStreamRecord;

typedef struct {
	MIRecord	parent;

	gchar*		token;
	gchar*		result_class;
	GHashTable*	results;		// key(gchar*) : value(MIValue)
} MIResultRecord;

typedef struct {
	GList*			out_of_bands;	// Record List
	MIResultRecord*	result_record;
} MIParseResult;

MIParseResult* mi_result_parse(const gchar* buf, gint len, gsize* bytes_read);

void mi_result_free(MIParseResult* v);

#endif//PUSS_MI_PROTOCOL_H

