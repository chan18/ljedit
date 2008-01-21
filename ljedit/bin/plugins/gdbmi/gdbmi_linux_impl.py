# gdbmi_linux_impl.py
# 
import os, sys, subprocess, time, re

import select

ASYNC_READ_BUFFER_SIZE = 4096

from gdbmi_protocol import parse_output, dump_output

re_pid = re.compile('''Using the running image of child process (\d+)\.''')

DEFAULT_TIMEOUT = 5.0

class BaseDriver:
	def __init__(self, target=None, args=None, working_directory=None, timeout=DEFAULT_TIMEOUT):
		self.target = target
		self.args = args
		self.working_directory = working_directory
		self.timeout = timeout
		self.pipe = None
		self.console = None
		self.child_running = False

	def __kill_child(self):
		ts = time.time()

		try:
			os.kill(self.child_pid, 9)
		except Exception:
			pass

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
		buf = ''
		fdr = [self.pipe.stdout.fileno()]
		(r, w, e) = select.select(fdr, [], [], 0.0)
		for fd in r:
			#print 'recv...'
			buf = os.read(fd, ASYNC_READ_BUFFER_SIZE)
			#print 'buf...', repr(buf)

		if len(buf) > 0:
			self.__recv_buf += buf

			while True:
				pos = self.__recv_buf.find('(gdb) \n')
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

	def __create_debug_console(self):
		if self.console:
			try:
				os.kill(self.console[0])
			except Exception:
				pass
			self.console = None

		sleep_command_line = 'sleep %s' % (80000 + os.getpid())
		pid = subprocess.Popen(['xterm', '-font', '-*-*-*-*-*-*-16-*-*-*-*-*-*-*', '-T', 'ljedit debug terminal', '-e', sleep_command_line]).pid
		#print pid
		tty = None

		ts = time.time()
		while tty==None and (time.time() - ts) < self.timeout:
			time.sleep(0.5)
			res = subprocess.Popen(['ps', 'x', '-o', 'tty,pid,command'], stdout=subprocess.PIPE).communicate()[0]
			#print res
	
			for r in res.split('\n'):
				try:
					if 'xterm' in r:
						continue
				
					if sleep_command_line in r:
						#print r
						s = r.strip().split()
						tty = s[0]
						break
				except Exception:
					pass

		if tty==None:
			try:
				os.kill(pid, 9)
			except Exception:
				pass
			raise Exception('create debug console failed!')

		self.console = pid, '/dev/' + tty
		self.__call('-inferior-tty-set %s' % self.console[1])

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

		self.pipe = subprocess.Popen( ['gdb', '--quiet', '--interpreter', 'mi', self.target]
			, stdin=subprocess.PIPE
			, stdout=subprocess.PIPE )
		self.child_pid = self.pipe.pid
		print 'gdb pid', self.child_pid

		self.__recv_output()					# ignore first (gdb) prompt
		#self.set_breakpoints()					# set breakpoints

	def start(self):
		self.__create_debug_console()
		self.__call('start')					# break at main, ignore return value
		pid = self.__fetch_child_pid()		# fetch child pid
		if pid!=None:
			self.child_pid = pid
		else:
			#print 'debug events way find child pid :', self.child_pid
			pass

	def run(self):
		self.start()
		self.__call('-exec-continue')
		#r = self.__call('-exec-run')
		#print 'run...', self.status, r

	def stop(self):
		if self.console:
			try:
				os.kill(self.console[0], 9)
			except Exception:
				pass
			self.console = None
			
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

