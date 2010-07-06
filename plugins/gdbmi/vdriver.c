// vdriver.c
// 

#include "vdriver.h"

#include <glib-object.h>

#include <memory.h>
#include <string.h>
#include <assert.h>

#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#endif

typedef enum { MI_VDRIVER_ST_NONE
	, MI_VDRIVER_ST_INIT				// gdb init
	, MI_VDRIVER_ST_TASK				// serials gdb operators
	, MI_VDRIVER_ST_WAIT				// gdb wait user input
} MIVDriverStatus;

typedef void (*MIVDriverTask)(MIVDriver* self, MIRecord* record, gint step);

struct _MIVDriver {
	// GDB Process
	GPid		pid;
	GIOChannel*	stdin_channel;
	GIOChannel*	stdout_channel;
	guint		stdin_event_handle;
	guint		stdout_event_handle;

	GString*	send_buffer;
	GString*	recv_buffer;

	// Target Process
	MITargetSetup*	target_setup;
	GPid			target_pid;
	MITargetStatus	target_status;

	// Private
	MIVDriverStatus	status;

	MIVDriverTask	task_func;
	gint			task_step;

	GRegex*			re_debugevents_pid;

	// Callbacks
	gpointer				cb_tag;
	GFreeFunc				cb_tag_free;

	MICommandMonitor		cb_command_monitor;
	MIMessageMonitor		cb_message_monitor;
	MITargetStatusChanged	cb_target_status_changed;
};

MITargetSetup* mi_target_setup_new() {
	return g_new0(MITargetSetup, 1);
}

void mi_target_setup_clear(MITargetSetup* setup) {
	if( setup ) {
		g_free(setup->gdb);
		g_free(setup->exec);
		g_free(setup->args);
		g_free(setup->working_dir);
		g_strfreev(setup->env);
		memset(setup, 0, sizeof(MITargetSetup));
	}
}

void mi_target_setup_free(MITargetSetup* setup) {
	mi_target_setup_clear(setup);
	g_free(setup);
}

static void mi_vdriver_set_target_status(MIVDriver* self, MITargetStatus status, const MIRecord* record) {
	self->target_status = status;

	if( self->cb_target_status_changed )
		(*(self->cb_target_status_changed))(self, status, record, self->cb_tag);
}

MITargetStatus	mi_vdriver_get_target_status(MIVDriver* self) {
	return self->target_status;
}

static void mi_vdriver_final(MIVDriver* self) {
	HANDLE hProcess;

	if( self->target_pid && self->target_status==MI_TARGET_ST_RUNNING ) {
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)self->target_pid);
		if( hProcess ) {
			TerminateProcess(hProcess, -1);
			CloseHandle(hProcess);
		}
	}
	self->target_pid = 0;
	mi_vdriver_set_target_status(self, MI_TARGET_ST_NONE, 0);
	self->status = MI_VDRIVER_ST_NONE;

	if( self->stdin_event_handle ) {
		g_source_remove(self->stdin_event_handle);
		self->stdin_event_handle = 0;
	}

	if( self->stdout_event_handle ) {
		g_source_remove(self->stdout_event_handle);
		self->stdout_event_handle = 0;
	}

	if( self->stdin_channel ) {
		g_io_channel_shutdown(self->stdin_channel, FALSE, 0);
		g_io_channel_unref(self->stdin_channel);
		self->stdin_channel = 0;
	}

	if( self->stdout_channel ) {
		g_io_channel_shutdown(self->stdout_channel, FALSE, 0);
		g_io_channel_unref(self->stdout_channel);
		self->stdout_channel = 0;
	}

	if( self->pid )
		g_spawn_close_pid(self->pid);

	g_string_assign(self->send_buffer, "");
	g_string_assign(self->recv_buffer, "");
}

static void mi_vdriver_send(MIVDriver* self, const gchar* buf, gsize len);

static void mi_vdriver_target_status_check(MIVDriver* self, MIRecord* record) {
	if( !record )
		return;

	switch( record->type ) {
	case '*':
		{
			MIAsyncRecord* p = (MIAsyncRecord*)record;
			if( !p->async_class )
				break;

			else if( g_str_equal(p->async_class, "stopped") ) {
				MIValue* reason;
				mi_vdriver_set_target_status(self, MI_TARGET_ST_STOPPED, record);
				reason = p->results ? g_hash_table_lookup(p->results, "reason") : 0;
				if( reason && reason->type=='c' && reason->v_const && g_str_equal(reason->v_const, "exited") )
					mi_vdriver_command_stop(self);
			}
		}
		break;

	case '^':
		{
			MIResultRecord* p = (MIResultRecord*)record;
			if( !p->result_class )
				break;
			
			else if( g_str_equal(p->result_class, "done") )
				mi_vdriver_set_target_status(self, MI_TARGET_ST_DONE, record);
			
			else if( g_str_equal(p->result_class, "running") )
				mi_vdriver_set_target_status(self, MI_TARGET_ST_RUNNING, record);
			
			else if( g_str_equal(p->result_class, "connected") )
				mi_vdriver_set_target_status(self, MI_TARGET_ST_CONNECTED, record);
			
			else if( g_str_equal(p->result_class, "error") )
				mi_vdriver_set_target_status(self, MI_TARGET_ST_ERROR, record);
			
			else if( g_str_equal(p->result_class, "exit") )
				mi_vdriver_set_target_status(self, MI_TARGET_ST_EXIT, record);
		}
		break;
	}
}

static void mi_vdriver_cb_recv(MIVDriver* self, const gchar* line, gsize size) {
	MIRecord* record;
	// g_print("RECV : %s\n", line);

	record = mi_record_parse(line, size);

	if( self->cb_message_monitor )
		(*(self->cb_message_monitor))(self, record, line, self->cb_tag);

	mi_vdriver_target_status_check(self, record);

	switch( self->status ) {
	case MI_VDRIVER_ST_NONE:
	case MI_VDRIVER_ST_INIT:
		break;

	case MI_VDRIVER_ST_TASK:
		assert( self->task_func );
		(*(self->task_func))(self, record, self->task_step);
		break;

	default:
		break;
	}

	mi_record_free(record);
}

static gboolean mi_vdriver_on_stdin_events( GIOChannel* source
		, GIOCondition condition
		, MIVDriver* self)
{
	GIOStatus st;
	gsize written = 0;

	if( condition & (G_IO_ERR | G_IO_HUP) )
		return FALSE;

	st = g_io_channel_write_chars( source
				, self->send_buffer->str
				, self->send_buffer->len
				, &written
				, 0 );

	g_string_erase(self->send_buffer, 0, (gssize)written);

	if( st==G_IO_STATUS_ERROR )
		return FALSE;

	return self->send_buffer->len > 0;
}

static gboolean mi_vdriver_on_stdout_events( GIOChannel* source
		, GIOCondition condition
		, MIVDriver* self)
{
	gchar* ps;
	gchar* pm;
	gchar* pe;
	gchar ch;
	gchar buf[8192];
	gsize len = 0;

	if( condition & (G_IO_ERR | G_IO_HUP) ) {
		if( self->status==MI_VDRIVER_ST_TASK ) {
			assert( self->task_func );
			self->target_status = MI_TARGET_ST_ERROR;
			(*(self->task_func))(self, 0, self->task_step);
		}
		mi_vdriver_final(self);
		return FALSE;
	}

	switch( g_io_channel_read_chars( source, buf, 8192, &len, 0 ) ) {
	case G_IO_STATUS_ERROR:
	case G_IO_STATUS_EOF:
		if( self->status==MI_VDRIVER_ST_TASK ) {
			assert( self->task_func );
			self->target_status = MI_TARGET_ST_ERROR;
			(*(self->task_func))(self, 0, self->task_step);
		}
		mi_vdriver_final(self);
		return FALSE;
	default:
		break;
	}

	//buf[len] = '\0';
	//g_print("STDOUT : %s\n", buf);

	g_string_append_len(self->recv_buffer, buf, len);

	ps = self->recv_buffer->str;
	pm = ps;
	pe = pm + self->recv_buffer->len;

	for( ; pm<pe; ++pm ) {
		if( *pm!='\n' )
			continue;

		++pm;
		ch = *pm;
		*pm = '\0';

		mi_vdriver_cb_recv(self, ps, (gsize)(pm-ps));

		*pm = ch;

		ps = pm;
	}

	g_string_erase(self->recv_buffer, 0, (gssize)(ps - self->recv_buffer->str));
	return TRUE;
}

static void mi_vdriver_send(MIVDriver* self, const gchar* buf, gsize len) {
	gboolean sign;
	if( len==0 )
		return;

	sign = (self->send_buffer->len==0);
	g_string_append_len(self->send_buffer, buf, len);

	if( sign ) {
		self->stdin_event_handle = g_io_add_watch( self->stdin_channel
			, G_IO_OUT | G_IO_ERR | G_IO_HUP
			, mi_vdriver_on_stdin_events
			, self );
	}
}

static gboolean mi_vdriver_prepare(MIVDriver* self) {
	GError* e = 0;
	gboolean res = FALSE;
	gint standard_input = 0;
	gint standard_output = 0;

	gchar* argv[8];

	argv[0] = "gdb";
	if(self->target_setup->gdb && self->target_setup->gdb[0])
		argv[0] = self->target_setup->gdb;

	argv[1] = "--interpreter=mi";
	argv[2] = "--quiet";
	argv[3] = self->target_setup->exec;
	argv[4] = 0;

	res = g_spawn_async_with_pipes( self->target_setup->working_dir
			, argv
			, self->target_setup->env
			, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL
			, 0
			, 0
			, &(self->pid)
			, &(standard_input)
			, &(standard_output)
			, 0
			, &e );

	if( e ) {
		g_printerr(e->message);
		g_error_free(e);
	}

	if( !res )
		return FALSE;

	self->stdin_channel  = g_io_channel_unix_new(standard_input);
	self->stdout_channel = g_io_channel_unix_new(standard_output);
	g_io_channel_set_encoding(self->stdin_channel, 0, 0);
	g_io_channel_set_encoding(self->stdout_channel, 0, 0);
	g_io_channel_set_buffered(self->stdin_channel, FALSE);
	g_io_channel_set_buffered(self->stdout_channel, FALSE);

	self->stdout_event_handle = g_io_add_watch( self->stdout_channel
		, G_IO_IN | G_IO_ERR | G_IO_HUP
		, mi_vdriver_on_stdout_events
		, self );

	return TRUE;
}

static void mi_vdriver_send_command(MIVDriver* self, const gchar* cmd) {
	gchar* buf;

	if( !cmd || cmd[0]=='\0' )
		return;

	buf = g_strdup_printf("%s\n", cmd);	// if need : add token

	g_free(buf);
}

static void mi_vdriver_task_call(MIVDriver* self, gint step, const gchar* cmd) {
	self->task_step = step;

	if( !cmd || cmd[0]=='\0' )
		return;

	//mi_vdriver_send(self, token if need);
	mi_vdriver_send(self, cmd, (gsize)strlen(cmd));
	mi_vdriver_send(self, "\n", 1);

	if( self->cb_command_monitor )
		(*(self->cb_command_monitor))(self, cmd, self->cb_tag);
}

static void mi_vdriver_task_start(MIVDriver* self, MIVDriverTask func, gint step, const gchar* cmd) {
	assert( self->task_func==0 );

	self->status = MI_VDRIVER_ST_TASK;
	self->task_func = func;
	self->task_step = step;

	if( cmd )
		mi_vdriver_task_call(self, step, cmd);

	(*func)(self, 0, step);
}

static void mi_vdriver_task_finish(MIVDriver* self, MIVDriverStatus status) {
	self->task_func = 0;
	self->status = status;
}

MIVDriver* mi_vdriver_new() {
	MIVDriver* self = g_new0(MIVDriver, 1);
	self->status = MI_VDRIVER_ST_NONE;

	self->re_debugevents_pid = g_regex_new("gdb: kernel event for pid=(\\d+) tid=\\d+ code=CREATE_PROCESS_DEBUG_EVENT", 0, 0, 0);

	self->send_buffer = g_string_new(0);
	self->recv_buffer = g_string_new(0);

#ifdef G_OS_WIN32
#ifndef _DEBUG
	AllocConsole();
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
#endif

	return self;
}

void mi_vdriver_free(MIVDriver* self) {

#ifdef G_OS_WIN32
#ifndef _DEBUG
	FreeConsole();
#endif
#endif

	mi_vdriver_final(self);

	g_regex_unref(self->re_debugevents_pid);

	g_string_free(self->send_buffer, TRUE);
	g_string_free(self->recv_buffer, TRUE);

	if( self->cb_tag_free )
		(*(self->cb_tag_free))(self->cb_tag);

	g_free(self);
}

void mi_vdriver_set_callbacks( MIVDriver* self
		, MICommandMonitor cb_command_monitor
		, MIMessageMonitor cb_message_monitor
		, MITargetStatusChanged cb_target_status_changed
		, gpointer tag
		, GFreeFunc tag_free )
{
	if( self->cb_tag_free )
		(*(self->cb_tag_free))(self->cb_tag);
	self->cb_tag_free = tag_free;
	self->cb_tag = tag;

	self->cb_command_monitor = cb_command_monitor;
	self->cb_message_monitor = cb_message_monitor;
	self->cb_target_status_changed = cb_target_status_changed;
}

void mi_vdriver_set_target_setup( MIVDriver* self, MITargetSetup* setup ) {
	self->target_setup = setup;
}

MITargetSetup*	mi_vdriver_get_target_setup(MIVDriver* self) {
	return self->target_setup;
}

static void task_mi_vdriver_run(MIVDriver* self, MIRecord* record, gint step) {
	GMatchInfo* info;
	gchar* word;

	// g_print("RUN STEP : %d\n", step);
	switch( step ) {
	case 0:
		self->target_setup->succeed = FALSE;
		if( !mi_vdriver_prepare(self) ) {
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_NONE);
			break;
		}
		self->task_step = 100;
		break;
		
	case 100:
		if( !record )
			mi_vdriver_task_call(self, 200, "-gdb-set new-console on");
		break;

	case 200:
		//if self.args:
		//	self.__call('-exec-arguments %s' % self.args)
		//if self.working_directory:
		//	self.__call('-environment-cd %s' % self.working_directory)

		//mi_vdriver_task_call(self, 201, "-exec-arguments %s");
		//mi_vdriver_task_call(self, 202, "-environment-cd %s");

		if( !record )
			mi_vdriver_task_call(self, 300, "-gdb-set debugevents on");
		break;
		
	case 300:
		if( !record )
			mi_vdriver_task_call(self, 400, "-exec-run");
		break;
		
	case 400:
		// running
		if( !record )
			self->task_step = 401;
		break;
		
	case 401:
		if( !record ) {
			mi_vdriver_task_call(self, 500, "-gdb-set debugevents off");
			break;
		}

		if( self->target_status==MI_TARGET_ST_ERROR ) {
			mi_vdriver_task_call(self, 402, "-gdb-exit");
			break;
		}

		if( record->type=='~' && g_regex_match(self->re_debugevents_pid, ((MIStreamRecord*)record)->str, 0, &info) ) {
			word = g_match_info_fetch(info, 1);

			// g_print("word:%s", word);
			self->target_pid = atol(word);
			self->target_setup->succeed = TRUE;
			// g_print("YYYYYYYYYY : %d\n", self->target_pid);

			g_free(word);
			g_match_info_free(info);
		}
		break;

	case 402:
		if( !record ) {
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_NONE);
			mi_vdriver_set_target_status(self, MI_TARGET_ST_NONE, record);
		}
		break;

	case 500:
		if( !record ) {
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_WAIT);
			// g_print("WHEN STOP : %d\n", self->target_pid);
		}
		break;
	}
}

static void task_mi_vdriver_stop(MIVDriver* self, MIRecord* record, gint step) {
	// g_print("STOP STEP : %d\n", step);
	switch( step ) {
	case 0:
		if( self->target_pid && self->target_status==MI_TARGET_ST_RUNNING )
			mi_vdriver_command_pause(self);

		mi_vdriver_task_call(self, 100, "-gdb-exit");
		break;

	case 100:
		if( !record )
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_NONE);
		break;
	}
}

static void task_mi_vdriver_normal(MIVDriver* self, MIRecord* record, gint step) {
	// g_print("NORMAL STEP : %d\n", step);
	switch( step ) {
	case 0:
		self->task_step = 100;
		break;
		
	case 100:
		if( !record )
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_WAIT);
		break;
	}
}

gboolean mi_vdriver_command_run(MIVDriver* self) {
	if( self->status != MI_VDRIVER_ST_NONE )
		return FALSE;

	if( !self->target_setup )
		return FALSE;

	self->status = MI_VDRIVER_ST_INIT;
	self->task_func = 0;
	mi_vdriver_task_start(self, task_mi_vdriver_run, 0, 0);
	return TRUE;
}

gboolean mi_vdriver_command_pause(MIVDriver* self) {
	HANDLE hProcess;

	if( !self->target_pid )
		return FALSE;

	if( self->target_status==MI_TARGET_ST_RUNNING ) {
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)self->target_pid);
		if( hProcess ) {
			DebugBreakProcess(hProcess);
			CloseHandle(hProcess);
		}
	}

	return TRUE;
}

gboolean mi_vdriver_command_stop(MIVDriver* self) {
	if( self->status==MI_VDRIVER_ST_TASK ) {
		self->status = MI_VDRIVER_ST_WAIT;
		self->task_func = 0;	// force finish last task
	}

	if( self->status==MI_VDRIVER_ST_WAIT )
		mi_vdriver_task_start(self, task_mi_vdriver_stop, 0, 0);

	return TRUE;
}

gboolean mi_vdirver_command_send(MIVDriver* self, gchar* cmd, ...) {
	va_list args;
	gchar* buf;

	if( !self->target_pid || self->status!=MI_VDRIVER_ST_WAIT )
		return FALSE;

	va_start(args, cmd);
	buf = g_strdup_vprintf(cmd, args);
	va_end (args);
	mi_vdriver_task_start(self, task_mi_vdriver_normal, 0, buf);
	g_free(buf);
	return TRUE;
}

