// vdriver.c
// 

#include "vdriver.h"

#include <memory.h>
#include <io.h>
#include <assert.h>

#include "protocol.h"

typedef enum { MI_VDRIVER_ST_NONE
	, MI_VDRIVER_ST_INIT				// gdb init
	, MI_VDRIVER_ST_TASK				// serials gdb operators
	, MI_VDRIVER_ST_WAIT				// gdb wait user input
	, MI_VDRIVER_ST_RUNNING			// gdb running
} MIVDriverStatus;

typedef void (*MIVDriverTask)(MIVDriver* self, MIParseResult* res, gint step);

struct _MIVDriver {
	// GDB Process
	GPid		pid;
	GIOChannel* stdin_channel;
	GIOChannel* stdout_channel;

	GString*	send_buffer;
	gint		send_offset;

	GString*	recv_buffer;

	// Target Process
	GPid			child_pid;

	// Private
	MIVDriverStatus	status;

	MIVDriverTask	task_func;
	gint			task_step;

	MIParseResult	current_result; 
};

static void mi_vdriver_send(MIVDriver* self, gchar* buf, gsize len);

static void mi_vdriver_cb_sent(MIVDriver* self, const gchar* cmd, gsize len) {
	g_print("SENT : %s\n", cmd);
}

static void mi_vdriver_cb_recv(MIVDriver* self, const gchar* res, gsize len) {
	MIParseResult* r = mi_result_parse(res, len, 0);
	g_print("RECV : %s\n", res);

	switch( self->status ) {
	case MI_VDRIVER_ST_NONE:
	case MI_VDRIVER_ST_INIT:
		break;

	case MI_VDRIVER_ST_TASK:
		assert( self->task_func );
		(*(self->task_func))(self, r, self->task_step);
		break;

	case MI_VDRIVER_ST_WAIT:
		break;

	case MI_VDRIVER_ST_RUNNING:
		break;
	}

	mi_result_free(r);
}

static gboolean mi_vdirver_on_stdin_events( GIOChannel* source
		, GIOCondition condition
		, MIVDriver* self)
{
	GIOError err;
	gchar* ps;
	gchar* pm;
	gchar* pe;
	gsize written = 0;

	if( condition & (G_IO_ERR | G_IO_HUP) )
		return FALSE;

	err = g_io_channel_write( source
				, self->send_buffer->str + self->send_offset
				, self->send_buffer->len - self->send_offset
				, &written );

	if( written ) {
		ps = self->send_buffer->str;
		pm = ps + self->send_offset;
		pe = pm + written;
		for( ; pm < pe; ++pm ) {
			if( *pm=='\n' ) {
				*pm = '\0';

				mi_vdriver_cb_sent(self, ps, pm-ps);

				self->send_offset = (pe - pm - 1);
				ps = pm + 1;
			}
		}

		g_string_erase(self->send_buffer, 0, pm - self->send_buffer->str);
	}

	if( err && err!=G_IO_ERROR_AGAIN )
		return FALSE;

	return self->send_buffer->len > 0;
}

static gboolean mi_vdirver_on_stdout_events( GIOChannel* source
		, GIOCondition condition
		, MIVDriver* self)
{
	GIOError err;
	gchar* ps;
	gchar* pm;
	gchar* pe;
	gchar ch;
	gboolean newline;
	gchar buf[8192];
	gsize len = 0;

	if( condition & (G_IO_ERR | G_IO_HUP) )
		return FALSE;

	err = g_io_channel_read( source, buf, 8192, &len );
	if( err && err!=G_IO_ERROR_AGAIN )
		return FALSE;
	if( len==0 )
		return FALSE;

	buf[len] = '\0';
	g_print("STDOUT : %s\n", buf);

	g_string_append_len(self->recv_buffer, buf, len);

	ps = self->recv_buffer->str;
	pm = ps;
	pe = pm + self->recv_buffer->len;
	newline = TRUE;
	for( ; pm<pe; ++pm ) {
		if( !newline ) {
			if( *pm=='\n' )
				newline = TRUE;
			continue;
		}

		newline = FALSE;
		if( (pe - pm) < 5 )
			break;

		if( memcmp(pm, "(gdb)", 5)!=0 )
			continue;

		pm += 5;
		while( (pm<pe) && *pm==' ' )
			++pm;

		if( (pm<pe) && *pm=='\r' )
			++pm;

		if( (pm<pe) && *pm=='\n' )
			++pm;
		else
			break;

		ch = *pm;
		*pm = '\0';
		g_print("OO : %s\n", ps);

		mi_vdriver_cb_recv(self, ps, pm-ps);

		*pm = ch;

		ps = pm;
		newline = TRUE;
	}

	g_string_erase(self->recv_buffer, 0, ps - self->recv_buffer->str);
	return TRUE;
}

static void mi_vdriver_send(MIVDriver* self, gchar* buf, gsize len) {
	gboolean sign;
	if( len==0 )
		return;

	sign = (self->send_buffer->len==0);
	g_string_append_len(self->send_buffer, buf, len);

	if( sign ) {
		guint res = g_io_add_watch( self->stdin_channel
			, G_IO_OUT | G_IO_ERR | G_IO_HUP
			, mi_vdirver_on_stdin_events
			, self );
	}
}

static gboolean mi_vdriver_prepare(MIVDriver* self) {
	GError* e = 0;
	gboolean res = FALSE;
	gint standard_input = 0;
	gint standard_output = 0;

	gchar* argv[8];
	gchar* envp[8];

	argv[0] = "D:\\mingw32\\bin\\gdb.exe";
	argv[1] = "--interpreter=mi";
	argv[2] = "--quiet";
	argv[3] = "d:\\t.exe";
	argv[4] = 0;

	envp[0] = 0;

	res = g_spawn_async_with_pipes( "D:\\puss\\bin"
			, argv
			, 0	//envp
			, 0
			, 0
			, 0
			, &self->pid
			, &standard_input
			, &standard_output
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
	g_io_channel_set_encoding(self->stdin_channel, 0, 0);

	g_io_add_watch( self->stdout_channel
		, G_IO_IN | G_IO_ERR | G_IO_HUP
		, mi_vdirver_on_stdout_events
		, self );

	self->send_buffer = g_string_new(0);
	self->recv_buffer = g_string_new(0);

	return TRUE;
}

static void mi_vdriver_call(MIVDriver* self, const gchar* cmd) {
	if( !cmd || cmd[0]=='\0' )
		return;

	// token
	//mi_vdriver_send((MIVDriver*)self, token, strlen(token));

	// cmd
	mi_vdriver_send((MIVDriver*)self, cmd, strlen(cmd));

	// LF
	mi_vdriver_send((MIVDriver*)self, "\n", 1);
}

static void mi_vdriver_task_start(MIVDriver* self, MIVDriverTask func, gint step) {
	assert( self->task_func==0 );

	self->status = MI_VDRIVER_ST_TASK;
	self->task_func = func;
	self->task_step = step;

	(*func)(self, 0, step);
}

static void mi_vdriver_task_finish(MIVDriver* self, MIVDriverStatus status) {
	self->task_func = 0;
	self->status = status;
}

static void mi_vdriver_task_call(MIVDriver* self, gint step, const gchar* cmd) {
	self->task_step = step;
	mi_vdriver_call(self, cmd);
}

static void mi_vdriver_task_callf(MIVDriver* self, gint step, const gchar* fmt, ...) {
	gchar* buffer;
	va_list args;

	va_start(args, fmt);
	buffer = g_strdup_vprintf(fmt, args);
	va_end (args);

	self->task_step = step;
	mi_vdriver_call(self, buffer);
	g_free(buffer);
}

MIVDriver* mi_vdriver_new() {
	MIVDriver* self = g_new0(MIVDriver, 1);
	self->status = MI_VDRIVER_ST_NONE;
}

void mi_vdriver_free(MIVDriver* self) {
	if( self->stdin_channel )
		g_io_channel_close(self->stdin_channel);
	if( self->stdout_channel )
		g_io_channel_close(self->stdout_channel);
	g_spawn_close_pid(self->pid);

	g_string_free(self->send_buffer, TRUE);
	g_string_free(self->recv_buffer, TRUE);

	g_free(self);
}

static void task_mi_vdriver_start(MIVDriver* self, MIParseResult* res, gint step) {
	g_print("STEP : %d\n", step);
	switch( step ) {
	case 0:
		if( !mi_vdriver_prepare( (MIVDriver*)self ) ) {
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_NONE);
			break;
		}
		self->task_step = 100;
		break;
		
	case 100:
		mi_vdriver_task_call(self, 200, "-gdb-set new-console on");
		break;

	case 200:
		//if self.args:
		//	self.__call('-exec-arguments %s' % self.args)
		//if self.working_directory:
		//	self.__call('-environment-cd %s' % self.working_directory)

		//mi_vdriver_task_call(self, 201, "-exec-arguments %s");
		//mi_vdriver_task_call(self, 202, "-environment-cd %s");

		mi_vdriver_task_call(self, 300, "-gdb-set debugevents on");
		break;
		
	case 300:
		mi_vdriver_task_call(self, 400, "start");
		break;
		
	case 400:
		{
			GList* p;
			MIRecord* v;
			GRegex* re_debugevents_pid;
			GMatchInfo* info;
			gchar* word;

			re_debugevents_pid = g_regex_new("gdb: kernel event for pid=(\\d+) tid=\\d+ code=CREATE_PROCESS_DEBUG_EVENT", 0, 0, 0);
			for( p = res->out_of_bands; p; p=p->next ) {
				v = (MIRecord*)(p->data);
				//if( v->type=='~' )
				//	g_print("::: %s\n", ((MIStreamRecord*)v)->str);

				if( v->type=='~' && g_regex_match(re_debugevents_pid, ((MIStreamRecord*)v)->str, 0, &info) ) {
					word = g_match_info_fetch(info, 1);

					//g_print("word:%s", word);
					self->child_pid = atol(word);
					g_print("YYYYYYYYYY : %d\n", self->child_pid);

					g_free(word);
					g_match_info_free(info);
				}
			}
			g_regex_unref(re_debugevents_pid);

			mi_vdriver_task_call(self, 500, "-gdb-set debugevents off");
		}
		break;

	case 500:
		mi_vdriver_task_finish(self, MI_VDRIVER_ST_WAIT);
		g_print("WHEN STOP : %d\n", self->child_pid);
		break;
	}
}

static void task_mi_vdriver_run(MIVDriver* self, MIParseResult* res, gint step) {
	g_print("STEP : %d\n", step);
	switch( step ) {
	case 0:
		if( !mi_vdriver_prepare( (MIVDriver*)self ) ) {
			mi_vdriver_task_finish(self, MI_VDRIVER_ST_NONE);
			break;
		}
		self->task_step = 100;
		break;
		
	case 100:
		mi_vdriver_task_call(self, 200, "-gdb-set new-console on");
		break;

	case 200:
		//if self.args:
		//	self.__call('-exec-arguments %s' % self.args)
		//if self.working_directory:
		//	self.__call('-environment-cd %s' % self.working_directory)

		//mi_vdriver_task_call(self, 201, "-exec-arguments %s");
		//mi_vdriver_task_call(self, 202, "-environment-cd %s");

		mi_vdriver_task_call(self, 300, "-gdb-set debugevents on");
		break;
		
	case 300:
		mi_vdriver_task_call(self, 400, "-exec-run");
		break;
		
	case 400:
		// running
		self->task_step = 401;
		break;
		
	case 401:
		{
			GList* p;
			MIRecord* v;
			GRegex* re_debugevents_pid;
			GMatchInfo* info;
			gchar* word;

			re_debugevents_pid = g_regex_new("gdb: kernel event for pid=(\\d+) tid=\\d+ code=CREATE_PROCESS_DEBUG_EVENT", 0, 0, 0);
			for( p = res->out_of_bands; p; p=p->next ) {
				v = (MIRecord*)(p->data);
				//if( v->type=='~' )
				//	g_print("::: %s\n", ((MIStreamRecord*)v)->str);

				if( v->type=='~' && g_regex_match(re_debugevents_pid, ((MIStreamRecord*)v)->str, 0, &info) ) {
					word = g_match_info_fetch(info, 1);

					//g_print("word:%s", word);
					self->child_pid = atol(word);
					g_print("YYYYYYYYYY : %d\n", self->child_pid);

					g_free(word);
					g_match_info_free(info);
				}
			}
			g_regex_unref(re_debugevents_pid);

			mi_vdriver_task_call(self, 500, "-gdb-set debugevents off");
		}
		break;

	case 500:
		mi_vdriver_task_finish(self, MI_VDRIVER_ST_WAIT);
		g_print("WHEN STOP : %d\n", self->child_pid);
		break;
	}
}

gboolean mi_vdriver_command_start(MIVDriver* self) {
	if( self->status != MI_VDRIVER_ST_NONE )
		return FALSE;

	self->status = MI_VDRIVER_ST_INIT;
	mi_vdriver_task_start(self, task_mi_vdriver_start, 0);
	return TRUE;
}

gboolean mi_vdriver_command_run(MIVDriver* self) {
	if( self->status != MI_VDRIVER_ST_NONE )
		return FALSE;

	self->status = MI_VDRIVER_ST_INIT;
	mi_vdriver_task_start(self, task_mi_vdriver_run, 0);
	return TRUE;
}

#ifdef TEST_VDRIVER

int main(int argc, char* argv[]) {
	GMainLoop* main_loop;
	MIVDriver* drv;

	main_loop = g_main_loop_new(0, FALSE);

	drv = mi_vdriver_new();

	if( !mi_vdriver_command_run(drv) )
		return 1;

	g_main_loop_run(main_loop);

	mi_vdriver_free(drv);
	g_main_loop_unref(main_loop);

	return 0;
}

#endif//TEST_VDRIVER

