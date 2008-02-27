# gdbmi_protocol.py
# 

# <<<GDB/MI output format>>>
# 
#output ->
#	( out-of-band-record )* [ result-record ] "(gdb)" nl
#result-record ->
#	[ token ] "^" result-class ( "," result )* nl
#out-of-band-record ->
#	async-record | stream-record
#async-record ->
#	exec-async-output | status-async-output | notify-async-output
#exec-async-output ->
#	[ token ] "*" async-output
#status-async-output ->
#	[ token ] "+" async-output
#notify-async-output ->
#	[ token ] "=" async-output
#async-output ->
#	async-class ( "," result )* nl
#result-class ->
#	"done" | "running" | "connected" | "error" | "exit"
#async-class ->
#	"stopped" | others (where others will be added depending on the needs—this
#	is still in development).
#result ->
#	variable "=" value
#variable ->
#	string
#value -> const | tuple | list
#const -> c-string
#tuple -> "{}" | "{" result ( "," result )* "}"
#list -> "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
#stream-record ->
#	console-stream-output | target-stream-output | log-stream-output
#console-stream-output ->
#	"~" c-string
#target-stream-output ->
#	"@" c-string
#log-stream-output ->
#	"&" c-string
#nl -> CR | CR-LF
#token -> any sequence of digits

class ParseError(Exception):
	def __init(self, *args):
		Exception.__init__(self, *args)

class OutputParser:
	def parse(self, buf):
		self.buf = buf
		self.end  = len(buf)
		self.pos = 0
		
		self.out_of_band_records = []
		self.result_record = None
		
		self.do_parse()
		
		return self.out_of_band_records, self.result_record

	def do_parse(self):
		# output -> ( out-of-band-record )* [ result-record ] "(gdb)" nl
		token = self.parse_token()
		while self.is_out_of_band_record():
			self.parse_out_of_band_record(token)
			token = self.parse_token()
			
		if self.is_result_record():
			self.parse_result_record(token)
			
		self.parse_end_sign()

	def current(self):
		return self.buf[self.pos]

	def parse_token(self):
		if self.current().isdigit():
			ps = self.pos
			while self.pos < self.end:
				self.pos += 1
				if self.current().isdigit():
					continue
				break
			return self.buf[ps:self.pos]

	def parse_string(self):
		ps = self.pos
		if not self.current().isalpha():
			raise ParseError('Bad string')
			
		while self.pos < self.end:
			self.pos += 1
			ch = self.current()
			if ch.isalnum() or ch=='-' or ch=='_':
				continue
			break
			
		return self.buf[ps:self.pos]

	def is_CR_LF(self):
		return self.current() in ('\r', '\n')

	def parse_CR_LF(self):
		if self.current()=='\n':
			self.pos += 1
		elif self.buf[self.pos:self.pos+2]=='\r\n':
			self.pos += 2
		else:
			raise ParseError('Bad CR-LF')

	def is_out_of_band_record(self):
		return self.is_async_record() or self.is_stream_record()

	def is_async_record(self):
		return self.current() in ('*', '+', '=')

	def is_stream_record(self):
		return self.current() in ('~', '@', '&')

	def is_result_record(self):
		return self.current()=='^'

	def parse_end_sign(self):
		pe = self.pos + 5
		if self.buf[self.pos:pe]!='(gdb)':
			#print '<<<<<', repr(self.buf[self.pos:pe])
			raise ParseError('Not find end sign "(gdb)"')
		self.pos = pe
		while self.current()==' ':	# gdb 6.6
			self.pos += 1
		self.parse_CR_LF()

	def parse_out_of_band_record(self, token):
		if self.is_async_record():
			self.parse_async_record(token)
		elif self.is_stream_record():
			#assert token==None
			self.parse_stream_record(token)
		else:
			raise ParseError('Not async-record or stream-record')

	def parse_async_record(self, token):
		record = None
		ch = self.current()
		self.pos += 1
		
		if ch=='*':
			record = self.parse_async_output()
		elif ch=='+':
			record = self.parse_async_output()
		elif ch=='=':
			record = self.parse_async_output()
		else:
			raise ParseError('Not find async-record sign')

		if record!=None:
			self.out_of_band_records.append( (token, ch, record[0], record[1]) )

	def parse_async_output(self):
		# async-output -> async-class ( "," result )* nl
		async_class = self.parse_string()
		if self.is_CR_LF():
			results = None
		else:
			if self.current()!=',':
				raise ParseError('Not find "," when parse async-output')
			self.pos += 1
			results = self.parse_results()
		self.parse_CR_LF()
		
		return async_class, results

	def parse_stream_record(self, token):
		ch = self.current()
		#assert ch in ('~', '@', '&')
		self.pos += 1
		
		string = self.parse_const()
		self.out_of_band_records.append( (token, ch, string) )
		self.parse_CR_LF()

	def parse_result_record(self, token):
		#result-record -> [ token ] "^" result-class ( "," result )* nl
		if self.current()!='^':
			raise ParseError('Not find "^" before result-record')
		self.pos += 1
		
		result_class = self.parse_string()
		if self.is_CR_LF():
			results = None
		else:
			if self.current()!=',':
				raise ParseError('Not find "," when parse result-record')
			self.pos += 1
			results = self.parse_results()
		self.parse_CR_LF()
		
		self.result_record = token, result_class, results

	def is_value_sign(self):
		return self.current() in ('"', '{', '[')

	def parse_results(self):
		results = {}
		if not self.current() in (']', '}'):
			while True:
				name = self.parse_string()
				#print '.. name=',name
				
				if self.current()!='=':
					raise ParseError('Not find "=" after name, when parse result')
				self.pos += 1
				
				results[name] = self.parse_value()
				#print '.. value=', results[name]
				
				if self.current()!=',':
					break
				self.pos += 1
				
		return results

	def parse_value(self):
		if self.current()=='"':
			return self.parse_const()
		elif self.current()=='{':
			return self.parse_tuple()
		elif self.current()=='[':
			return self.parse_list()
		raise ParseError('Bad value!')

	def parse_values(self):
		values = []
		while True:
			ch = self.current()
			
			if ch==']':
				break
				
			if ch==',':
				self.pos += 1
				
			values.append(self.parse_value())
		return values

	def parse_const(self):
		ps = self.pos
		if self.current()!='"':
			raise ParseError('Bad const')
			
		while True:
			self.pos += 1
			ch = self.current()
			if ch=='"':
				self.pos += 1
				break
			if ch=='\\':
				self.pos += 1
			
		return self.buf[ps+1:self.pos-1]

	def parse_tuple(self):
		if not self.current()=='{':
			raise ParseError('Bad tuple')
		self.pos += 1
		
		if self.current()=='}':
			result = {}
		else:
			result = self.parse_results()
			if self.current()!='}':
				raise ParseError('Bad tuple')
		self.pos += 1
		return result

	def parse_list(self):
		if not self.current()=='[':
			raise ParseError('Bad list')
		self.pos += 1
		
		if self.current()==']':
			result = []
		else:
			if self.is_value_sign():
				result = self.parse_values()
			else:
				result = self.parse_results()
			if self.current()!=']':
				raise ParseError('Bad list')
		self.pos += 1
		return result

# <<<parse_output return value>>>
# 
# output = out-of-band-record*, result-record
# 
# out-of-band-record = token, */+/=, async-class, result*
#                    | token, ~/@/&, string
# 
# result-record = token, result-class, result*
#				| None
# 
# result = {variable:value}
# 
def parse_output(output):
	parser = OutputParser()
	try:
		return parser.parse(output)
	except:
		print '='*60
		print 'Error Output :', output
		print '      pos    :', output[:parser.pos], '^^^here^^^', output[parser.pos:]
		print '='*60
		raise

SPACE = '  '

def out_of_band_record_to_string(record, layer):
	return SPACE*layer + str(record)

def result_record_to_string(record, layer):
	return SPACE*layer + str(record)

def output_to_string(output):
	retval = 'output :\n'
	if len(output[0])!=0:
		retval += (SPACE + 'out-of-band-records :\n')
		for r in output[0]:
			retval += (out_of_band_record_to_string(r, 2) + '\n')

	if output[1]!=None:
		retval += (SPACE + 'result-record :\n')
		retval += (result_record_to_string(output[1], 2) + '\n')

	return retval

def dump_output(output):
	print '='*60
	try:
		print output_to_string(output)
	except:
		print 'Error Output :', output
		raise

# test
# 
if __name__=='__main__':
	print '='*16, 'test', '='*16

	line = '''^error,msg="Undefined mi command: exec-show-arguments (missing implementation)"\r\n(gdb) \r\n'''
	print parse_output(line)

	line = '''^done,bkpt={number="1",type="breakpoint",disp="keep",enabled="y",addr="0x004010f6",func="main",file="demo.c",line="23",times="0"}\r\n(gdb)\r\n'''
	print parse_output(line)
