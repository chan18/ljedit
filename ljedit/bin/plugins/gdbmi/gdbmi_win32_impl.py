# gdbmi_win32_impl.py
# 
import os, sys, subprocess, time, re
import ctypes, msvcrt

ASYNC_READ_BUFFER_SIZE = 4096

PIPE = subprocess.PIPE

class AsyncPopen(subprocess.Popen):
	def __init__(self, *args, **kargs):
		subprocess.Popen.__init__(self, *args, **kargs)
		
		# for __async_recv
		self.__async_s = ctypes.create_string_buffer(ASYNC_READ_BUFFER_SIZE)
		self.__async_n = ctypes.c_uint32(0)	# keep zero
		self.__async_r = ctypes.c_uint32(0)
		self.__async_a = ctypes.c_uint32(0)
		self.__async_m = ctypes.c_uint32(0)
		self.__async_peek = ctypes.windll.kernel32.PeekNamedPipe
		self.__async_read = ctypes.windll.kernel32.ReadFile
		
	def async_recv_stdout(self):
		return self.__async_recv(self.stdout)
		
	def async_recv_stderr(self):
		return self.__async_recv(self.stderr)
		
	def __async_recv(self, pipe):
		ret = ''
		handle = msvcrt.get_osfhandle(pipe.fileno())
		hr = self.__async_peek( handle
			, self.__async_s
			, self.__async_n
			, ctypes.byref(self.__async_r)
			, ctypes.byref(self.__async_a)
			, ctypes.byref(self.__async_m) )
		if hr and self.__async_a.value > 0:
			if self.__async_a.value > ASYNC_READ_BUFFER_SIZE:
				self.__async_a.value = ASYNC_READ_BUFFER_SIZE
			#print self.__async_a.value
			
			if self.__async_read(handle, self.__async_s, self.__async_a, ctypes.byref(self.__async_m), 0):
				ret = self.__async_s.value[:self.__async_a.value]
				#assert self.__async_m.value==self.__async_a.value
				#print self.__async_m
				#print self.__async_s
		return ret

def terminal_process(pid):
	PROCESS_TERMINATE = 1
	handle = ctypes.windll.kernel32.OpenProcess(PROCESS_TERMINATE, False, pid)
	ctypes.windll.kernel32.TerminateProcess(handle, -1)
	ctypes.windll.kernel32.CloseHandle(handle)

from gdbmi_protocol import parse_output, dump_output

re_pid = re.compile('''Using the running image of child thread (\d+)\.''')
re_debugevents_pid = re.compile('''gdb: kernel event for pid=(\d+) tid=\d+ code=CREATE_PROCESS_DEBUG_EVENT''')

DEFAULT_TIMEOUT = 5.0

class BaseDriver:
	def __init__(self, target=None, args=None, working_directory=None, timeout=DEFAULT_TIMEOUT):
		self.target = target
		self.args = args
		self.working_directory = working_directory
		self.timeout = timeout
		self.pipe = None
		self.child_pid = None
		self.child_running = False

	def __kill_child(self):
		ts = time.time()
		terminal_process(self.child_pid)
		while self.child_running and (time.time() - ts) < self.timeout:
			time.sleep(0.1)
			self.__recv()

	def __send(self, cmd):
		#print 'send :', cmd
		try:
			self.handle_send(cmd)
		except Exception, e:
			print 'handle_send error :', e

		self.pipe.stdin.write(cmd+'\n')

	def __recv(self):
		buf = self.pipe.async_recv_stdout()

		if len(buf) > 0:
			self.__recv_buf += buf
			
			# use "set debugevents on" then start/run find child_pid
			# 
			if self.child_pid==None:
				r = re_debugevents_pid.search(self.__recv_buf)
				if r:
					self.child_pid = int(r.group(1))
					self.__send('set debugevents off')

			while True:
				pos = self.__recv_buf.find('(gdb) \r\n')
				if pos < 0:
					break
				pos += 8
				packet = self.__recv_buf[:pos]
				self.__recv_buf = self.__recv_buf[pos:]
				
				output = parse_output(packet)
				#print 'RRRRRRRRRR :', packet
				self.__recv_queue.append((output, packet))

		if len(self.__recv_queue) > 0:
			output, packet = self.__recv_queue.pop(0)
			if output[1]:
				if output[1][1]=='running':
					self.child_running = True
			for r in  output[0]:
				if r[1]=='*' and r[2]=='stopped':
					self.child_running = False
					try:
						if r[3]['reason']=='exited':
							self.child_pid = None
					except Exception:
						pass
			try:
				self.handle_recv(output, packet)
			except Exception, e:
				print 'handle_recv error :', e

			return output

	def __recv_output(self):
		ts = time.time()
		while (time.time() - ts) < self.timeout:
			time.sleep(0.1)
			output = self.__recv()
			if output:
				return output

	def __call(self, cmd):
		self.__call_index += 1
		index = str(self.__call_index)

		self.__send(index+cmd)

		ts = time.time()
		while (time.time() - ts) < self.timeout:
			time.sleep(0.1)
			output = self.__recv()
			if output==None or output[1]==None:
				continue
			if output[1][0]==index:
				return output

	def __fetch_child_pid(self):
		# use "info program" find child pid
		# 
		out_of_band_records, result_record = self.__call('info program')
		for s in out_of_band_records:
			if s[1]!='~':			# console output
				continue
			r = re_pid.search(s[2])
			if r:
				return int(r.group(1))

	def prepare(self):
		if self.pipe:
			self.stop()

		self.pipe = None
		self.child_running = False
		self.child_pid = None
		self.__call_index = 0
		self.__send_queue = []
		self.__recv_queue = []
		self.__recv_buf = ''

		if self.target==None:
			raise Exception('Not set debug target!')

		if not os.path.exists(self.target):
			raise Exception('Not find debug target file!')

		self.pipe = AsyncPopen( ['gdb', '--quiet', '--interpreter', 'mi', self.target]
			, stdin=PIPE
			, stdout=PIPE
			, shell=True )

		self.__recv_output()					# ignore first (gdb) prompt
		self.__call('-gdb-set new-console on')	# ignore return value

		if self.args:
			self.__call('-exec-arguments %s' % self.args)

		if self.working_directory:
			self.__call('-environment-cd %s' % self.working_directory)

		#self.set_breakpoints()					# set breakpoints

	def start(self):
		self.__call('-gdb-set debugevents on')	# ignore return value, use debug events
		self.__call('start')					# break at main, ignore return value
		if self.child_pid==None:
			pid = self.__fetch_child_pid()			# fetch child pid
			if pid==None:
				raise Exception('Not find child pid!')
			self.child_pid = pid
		else:
			#print 'debug events way find child pid :', self.child_pid
			pass

	def run(self):
		if self.child_pid:
			if self.child_running:
				self.__kill_child()

		self.child_pid = None
		self.child_running = False
		self.__call('-gdb-set debugevents on')	# ignore return value, use debug events
		self.__call('-exec-run')

	def stop(self):
		if self.child_pid:
			if self.child_running:
				self.__kill_child()

		self.child_pid = None
		self.child_running = False

		if self.pipe:
			self.__send('-gdb-exit')

			#self.pipe.wait()
			ts = time.time()
			while (time.time() - ts) < self.timeout:
				if self.pipe.poll()==None:
					time.sleep(0.1)
				else:
					self.pipe = None
					break

		if self.pipe:
			terminal_process(self.pipe.pid)
			self.pipe = None

	def call(self, cmd):
		if not self.child_running:
			return self.__call(cmd)

	def dispatch(self):
		while self.pipe:
			if self.__recv()==None:
				break

	def handle_send(self, command):
		pass

	def handle_recv(self, output, packet):
		pass
