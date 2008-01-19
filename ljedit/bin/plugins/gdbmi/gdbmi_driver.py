# gdbmi_win32_impl.py
# 

import sys, time

if sys.platform=='win32':
	from gdbmi_win32_impl import BaseDriver
else:
	from gdbmi_linux_impl import BaseDriver

class Driver(BaseDriver):
	def __init__(self, *args, **kwargs):
		BaseDriver.__init__(self, *args, **kwargs)

	def __call_with_arg_or_arglist(self, cmd, arg_or_arglist):
		if arg_or_arglist is None:
			pass
		elif isinstance(arg_or_arglist, (tuple, list)):
			for arg in arg_or_arglist:
				cmd += ' %s' % arg
		else:
			cmd += ' %s' % arg_or_arglist
		return self.__call(cmd)

	#######################################################################
	# break commands
	#######################################################################

	def break_after(self, breakpoint, count):
		'''The breakpoint number is not in effect until it has been hit count times.'''
		return self.call('-break-after %s %s' % (breakpoint, count))

	def break_condition(self, breakpoint, expr):
		'''Breakpoint number will stop the program only if the condition in expr is true.'''
		return self.call('-break-condition %s %s' % (breakpoint, expr))

	def break_delete(self, breakpoints):
		'''Delete the breakpoint(s) whose number(s) are specified in the argument list.'''
		return self.__call_with_arg_or_arglist('-break-delete', breakpoints)

	def break_disable(self, breakpoints):
		'''Disable the named breakpoint(s). The field ‘enabled’ in the break list is now set to ‘n’for the named breakpoint(s).'''
		return self.__call_with_arg_or_arglist('-break-disable', breakpoints)

	def break_enable(self, breakpoints):
		'''Enable (previously disabled) breakpoint(s).'''
		return self.__call_with_arg_or_arglist('-break-enable', breakpoints)

	def break_info(self, breakpoint):
		'''Get information about a single breakpoint.'''
		return self.call('-break-info %s' % breakpoint)

	def break_insert(self, where, option=None):
		'''-break-insert [-t] [-h] [-r] [-c condition] [-i ignore-count] [-p thread] [line | addr]\n'''\
		'''If specified, line, can be one of:\n'''\
		''' > function\n'''\
		''' > filename:linenum\n'''\
		''' > filename:function\n'''\
		''' > *address\n'''
		cmd = '-break-insert'
		if option:
			cmd += ' '
			cmd += option
		cmd += ' '
		cmd += where

	def break_list(self):
		'''Displays the list of inserted breakpoints, showing the following fields:\n'''\
		'''	"Number" number of the breakpoint\n'''\
		'''	"Type" type of the breakpoint: "breakpoint" or "watchpoint"\n'''\
		'''	"Disposition" : should the breakpoint be deleted or disabled when it is hit: "keep" or "nokeep"\n'''\
		'''	"Enabled" is the breakpoint enabled or no: "y" or "n"\n'''\
		'''	"Address" memory location at which the breakpoint is set\n'''\
		'''	"What" logical location of the breakpoint, expressed by function name, file name, line number\n'''\
		'''	"Times" number of times the breakpoint has been hit\n'''\
		'''If there are no breakpoints or watchpoints, the BreakpointTable body field is an empty list.'''
		return self.call('-break-list')

	def break_watch(self, witch, option=None):
		'''-break-watch [-a | -r]\n'''\
		'''Create a watchpoint. With the "-a" option it will create an access watchpoint, i.e., a'''\
		'''watchpoint that triggers either on a read from or on a write to the memory location. With'''\
		'''the "-r" option, the watchpoint created is a read watchpoint, i.e., it will trigger only when'''\
		'''the memory location is accessed for reading. Without either of the options, the watchpoint'''\
		'''created is a regular watchpoint, i.e., it will trigger when the memory location is accessed for writing.'''
		cmd = '-break-watch'
		if option:
			cmd += ' '
			cmd += option
		cmd += ' '
		cmd += witch

	#######################################################################
	# program context commands
	#######################################################################

	def exec_arguments(self, args):
		'''Set the inferior program arguments, to be used in the next "-exec-run".'''
		return self.call('-exec-arguments %s' % args)

	def exec_show_arguments(self):
		'''Print the arguments of the program.'''
		return self.call('-exec-show-arguments')

	def environment_cd(self, path):
		'''Set gdb working directory.'''
		return self.call('-environment-cd %s' % path)

	def environment_directory(self, pathdirs=None, reset=False):
		'''Add directories pathdir to beginning of search path for source files. If the "-r" option is '''\
		'''used, the search path is reset to the default search path. If directories pathdir are supplied '''\
		'''in addition to the "-r" option, the search path is first reset and then addition occurs as '''\
		'''normal. Multiple directories may be specified, separated by blanks. Specifying multiple '''\
		'''directories in a single command results in the directories added to the beginning of the '''\
		'''search path in the same order they were presented in the command. If blanks are needed as '''\
		'''part of a directory name, double-quotes should be used around the name. In the command '''\
		'''output, the path will show up separated by the system directory-separator character. The '''\
		'''directory-separator character must not be used in any directory name. If no directories are '''\
		'''specified, the current search path is displayed.'''
		# source directory
		# 
		cmd = '-environment-directory'
		if reset:
			cmd += ' -r'
		return self.__call_with_arg_or_arglist(cmd, pathdirs)

	def environment_path(self):
		'''Add directories pathdir to beginning of search path for object files. If the "-r" option '''\
		'''is used, the search path is reset to the original search path that existed at gdb start-up. '''\
		'''If directories pathdir are supplied in addition to the "-r" option, the search path is first '''\
		'''reset and then addition occurs as normal. Multiple directories may be specified, separated '''\
		'''by blanks. Specifying multiple directories in a single command results in the directories '''\
		'''added to the beginning of the search path in the same order they were presented in the '''\
		'''command. If blanks are needed as part of a directory name, double-quotes should be used '''\
		'''around the name. In the command output, the path will show up separated by the system '''\
		'''directory-separator character. The directory-separator character must not be used in any '''\
		'''directory name. If no directories are specified, the current path is displayed.'''
		# object directory
		# 
		cmd = '-environment-path'
		if reset:
			cmd += ' -r'
		return self.__call_with_arg_or_arglist(cmd, pathdirs)

	def environment_pwd(self):
		'''Show the current working directory.'''
		return self.call('-environment-pwd')

	#######################################################################
	# thread commands
	#######################################################################

	#def thread_info(self):
	#	pass

	def thread_list_all_threads(self):
		return self.call('info threads')

	def thread_list_ids(self):
		'''Produces a list of the currently known gdb thread ids. At the end of the list it also prints the total number of such threads.'''
		return self.call('-thread-list-ids')

	def thread_select(self, threadnum):
		'''Make threadnum the current thread. It prints the number of the new current thread, and the topmost frame for that thread.'''
		return self.call('-thread-select %s' % threadnum)

	#######################################################################
	# program execution commands
	#######################################################################

	def exec_continue(self):
		'''Resumes the execution of the inferior program until a breakpoint is encountered, or until the inferior exits.'''
		return self.call('-exec-continue')

	def exec_finish(self):
		'''Resumes the execution of the inferior program until the current function is exited. Displays the results returned by the function.'''
		return self.call('-exec-finish')

	def exec_interrupt(self):
		'''Interrupts the background execution of the target. Note how the token associated with '''\
		'''the stop message is the one for the execution command that has been interrupted. The '''\
		'''token for the interrupt itself only appears in the "^done" output. If the user is trying to '''\
		'''interrupt a non-running program, an error message will be printed.'''
		return self.call('-exec-interrupt')

	def exec_next(self):
		'''Resumes execution of the inferior program, stopping when the beginning of the next source line is reached.'''
		return self.call('-exec-next')

	def exec_next_instruction(self):
		'''Executes one machine instruction. If the instruction is a function call, continues until '''\
		'''the function returns. If the program stops at an instruction in the middle of a source line, '''\
		'''the address will be printed as well.'''
		return self.call('-exec-next-instruction')

	def exec_return(self):
		'''Makes current function return immediately. Doesn’t execute the inferior. Displays the new current frame.'''
		return self.call('-exec-return')

	def exec_run(self):
		'''Starts execution of the inferior from the beginning. The inferior executes until either a '''\
		'''breakpoint is encountered or the program exits. In the latter case the output will include '''\
		'''an exit code, if the program has exited exceptionally. '''
		return self.call('-exec-run')

	def exec_step(self):
		'''Resumes execution of the inferior program, stopping when the beginning of the next '''\
		'''source line is reached, if the next source line is not a function call. If it is, stop at the first '''\
		'''instruction of the called function.'''
		return self.call('-exec-step')

	def exec_step_instruction(self):
		'''Resumes the inferior which executes one machine instruction. The output, once gdb has '''\
		'''stopped, will vary depending on whether we have stopped in the middle of a source line or '''\
		'''not. In the former case, the address at which the program stopped will be printed as well'''
		return self.call('-exec-step-instruction')

	def exec_until(self, location=None):
		'''Executes the inferior until the location specified in the argument is reached. If there '''\
		'''is no argument, the inferior executes until a source line greater than the current one is '''\
		'''reached. The reason for stopping in this case will be "location-reached".'''
		cmd = '-exec-until'
		if location:
			cmd += ' '
			cmd += location
		return self.call(cmd)

	#######################################################################
	# static manipulation commands
	#######################################################################

	def static_info_frame(self):
		'''Get info on the selected frame.'''
		return self.call('-stack-info-frame')

	def stack_info_depth(self, max_depth=None):
		'''Return the depth of the stack. If the integer argument max-depth is specified, do not count beyond max-depth frames.'''
		cmd = '-stack-info-depth'
		if max_depth:
			cmd += ' %s' % max_depth
		return self.call(cmd)

	def stack_list_arguments(self, show_values=True, low_high_frame_tuple=None):
		'''Display a list of the arguments for the frames between low-frame and high-frame (inclusive). '''\
		'''If low-frame and high-frame are not provided, list the arguments for the whole call '''\
		'''stack. If the two arguments are equal, show the single frame at the corresponding level. '''\
		'''It is an error if low-frame is larger than the actual number of frames. On the other hand, '''\
		'''high-frame may be larger than the actual number of frames, in which case only existing '''\
		'''frames will be returned.\n'''\
		'''The show-values argument must have a value of 0 or 1. A value of 0 means that only '''\
		'''the names of the arguments are listed, a value of 1 means that both names and values of '''\
		'''the arguments are printed.'''
		cmd = '-stack-list-arguments %s' % ('1' if show_values else '0')
		if low_high_frame_tuple:
			cmd += ' %s %s' % low_high_frame_tuple
		return self.call(cmd)

	def stack_list_frames(self, low_high_frame_tuple=None):
		'''List the frames currently on the stack.'''
		cmd = '-stack-list-frames'
		if low_high_frame_tuple:
			cmd += ' %s %s' % low_high_frame_tuple
		return self.call(cmd)

	def stack_list_locals(self, print_values=1):
		'''Display the local variable names for the selected frame. If print-values is 0 or --no-values, '''\
		'''print only the names of the variables; if it is 1 or --all-values, print also their values; '''\
		'''and if it is 2 or --simple-values, print the name, type and value for simple data types and '''\
		'''the name and type for arrays, structures and unions. In this last case, a frontend can '''\
		'''immediately display the value of simple data types and create variable objects for other '''\
		'''data types when the user wishes to explore their values in more detail.'''
		return self.call('-stack-list-locals %s' % print_values)

	def stack_select_frame(self, framenum):
		'''Change the selected frame. Select a diiferent frame framenum on the stack.'''
		return self.call('-stack-select-frame %s' % framenum)

	#######################################################################
	# var objects commands
	#######################################################################

	# i don't known how to use theme, ignored!

	#######################################################################
	# data manipulation commands
	#######################################################################

	def data_disassemble(self, disassemble_range, mode=0):
		'''disass_range : (start_addr, end_addr)\n'''\
		'''             : (filename, linenum, lines)\n'''\
		'''mode         : 0 - (only disassembly)\n'''\
		'''             : 1 - (mixed source and disassembly)'''
		cmd = '-data-disassemble'
		if len(disass_range)==2:
			cmd += ' -s %s -e %s' % disassemble_range
		elif len(disass_range)==3:
			cmd += ' -f %s -l %s' % disassemble_range[:2]
			if disassemble_range[2]:
				cmd += ' -n %s' % disassemble_range[2]
		else:
			raise Exception('bad disassemble range format')
		cmd += ' -- %s' % ('0' if mode==0 else '1')
		return self.call('-data-disassemble')

	# TODO : other data manipulation commands

	#######################################################################
	# TODO : tracepoint commands
	#######################################################################

	#######################################################################
	# TODO : symbol query commands
	#######################################################################

	#######################################################################
	# TODO : file commands
	#######################################################################

	#######################################################################
	# TODO : target manipulation commands
	#######################################################################

	#######################################################################
	# miscellaneous commands
	#######################################################################

	def gdb_exit(self):
		'''Exit GDB immediately.'''
		return self.call('-gdb-exit')

	def gdb_set(self, var, value):
		'''Set an internal GDB variable.'''
		return self.call('-gdb-set $%s %s' % (var, value))

	def gdb_show(self, var):
		'''Show thre current value of GDB variable.'''
		return self.call('-gdb-show %s' % var)

	def gdb_version(self):
		'''Show version information for GDB. Used mostly in testing.'''
		return self.call('-gdb-version')

	def gdb_list_features(self):
		'''Returns a list of particular features of the MI protocol that this version of gdb implements. '''\
		'''A feature can be a command, or a new ﬁeld in an output of some command, or even an '''\
		'''important bug fix. While a frontend can sometimes detect presence of a feature at runtime, '''\
		'''it is easier to perform detection at debugger startup.'''
		return self.call('-list-features')

	def interpreter_exec(self, interpreter, command):
		'''Execute the specified command in the given interpreter.'''
		return self.call('-interpreter-exec %s %s' % (interpreter, command))

	def inferior_tty_set(self, tty):
		'''Set terminal for future runs of the program being debugged.'''
		return self.call('-inferior-tty-set %s' % tty)

	def inferior_tty_show(self, tty):
		'''Show terminal for future runs of program being debugged.'''
		return self.call('-inferior-tty-show')

	def enable_timings(self, enable=True):
		'''Toggle the printing of the wallclock, user and system times for an MI command as a '''\
		'''field in its output. This command is to help frontend developers optimize the performance '''\
		'''of their code. No argument is equivalent to "yes".'''
		return self.call('-enable-timings %s' % ('yes' if enable else 'no'))
