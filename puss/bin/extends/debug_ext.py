# debug_ext.py
# 
import gdbmi

import gobject, gtk

def append_button(vbox, title, clickcb):
	btn = gtk.Button(title)
	btn.connect('clicked', clickcb)
	vbox.pack_start(btn)

class GdbPanel(gtk.VBox, gdbmi.Driver):
	def __init__(self, main_window):
		gtk.VBox.__init__(self)
		gdbmi.Driver.__init__(self)
		self.main_window = main_window
		self.last_cmd = ()

		self.ui = """<ui>
			<toolbar name='Toolbar'>
				<toolitem action='setup'/>
				<separator/>
				<toolitem action='prepare'/>
				<separator/>
				<toolitem action='start'/>
				<toolitem action='run'/>
				<toolitem action='stop'/>
				<separator/>
				<toolitem action='next'/>
				<toolitem action='step'/>
				<toolitem action='continue'/>
				<!--
				<separator/>
				<toolitem action='Toggle'/>
				-->
				<separator/>
				<placeholder name='DebugPages'>
					<toolitem action='output'/>
					<toolitem action='locals'/>
				</placeholder>
			</toolbar>
		</ui>"""

		self.uimgr = gtk.UIManager()
		actiongroup = gtk.ActionGroup('LJDEBUG_ACTIONGROUP')
		actiongroup.add_actions(
			[ ('setup',     None,   'setup',     None,   'setup tip',     lambda *args : self.on_setup())
			, ('prepare',   None,   'prepare',   None,   'prepare tip',   lambda *args : self.prepare())
			, ('start',     None,   'start',     None,   'start tip',     lambda *args : self.start())
			, ('run',       None,   'run',       None,   'run tip',       lambda *args : self.run())
			, ('stop',      None,   'stop',      None,   'stop tip',      lambda *args : self.stop())
			, ('next',      None,   'next',      None,   'next tip',      lambda *args : self.call('-exec-next'))
			, ('step',      None,   'step',      None,   'step tip',      lambda *args : self.call('-exec-step'))
			, ('continue',  None,   'continue',  None,   'continue tip',  lambda *args : self.call('-exec-continue'))
			] )

		#actiongroup.add_toggle_actions([('Toggle', None, 'toggle', '<Control>t', 'Toggle tip', lambda *args : None)])
		actiongroup.add_radio_actions(
			[ ('output',    None,   'output',    '<control>e', 'output tip', 0)
        	, ('locals',    None,   'locals',    '<control>r', 'locals tip', 1)
        	], 0, self.on_change_page )

		self.uimgr.insert_action_group(actiongroup, 0)
		merge_id = self.uimgr.add_ui_from_string(self.ui)

		toolbar = self.uimgr.get_widget('/Toolbar')
		self.pack_start(toolbar, False)

		# pages
		self.buf = gtk.TextBuffer()
		self.command_tag = self.buf.create_tag(foreground='red')
		self.packet_tag = self.buf.create_tag(foreground='gray')
		self.output_tag = self.buf.create_tag(foreground='blue')
		self.info_tag = self.buf.create_tag(foreground='green')

		self.output_view = gtk.TextView(self.buf)
		output_page = gtk.ScrolledWindow()
		output_page.add(self.output_view)
		output_page.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)

		self.locals_view = gtk.TreeView()
		locals_page = gtk.ScrolledWindow()
		locals_page.add(self.locals_view)
		locals_page.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		
		self.nb = gtk.Notebook()
		self.nb.set_show_tabs(False)
		self.nb.append_page(output_page, None)
		self.nb.append_page(locals_page, None)

		self.pack_start(self.nb)

		self.cmd_entry = gtk.Entry()
		self.cmd_entry.connect('key_press_event', self.on_cmd_entry_key_press)
		self.pack_end(self.cmd_entry, False)

		gobject.timeout_add(100, self.on_dispatch_timer)

	def output_text(self, text, tag=None):
		iter = self.buf.get_end_iter()
		self.buf.insert_with_tags(iter, str(text), tag if tag else self.info_tag)
		gobject.idle_add(self.scroll_to_end)

	def handle_send(self, command):
		self.output_text(str(command) + '\n', self.command_tag)

	def handle_recv(self, output, packet):
		self.output_text(str(packet), self.packet_tag)
		self.output_text(gdbmi.output_to_string(output) + '\n\n', self.output_tag)

		obs, res = output
		for ob in obs:
			if ob[1]=='*':
				if ob[2]=='stopped':
					rs = ob[3]
					if rs['reason'] in ('end-stepping-range', ''):
						filename = rs['frame']['fullname']
						line = int(rs['frame']['line'])
						self.update_stack_list_locals()
						self.handle_debug_stopped(filename, line)
					
				elif ob[2]=='exited':
					pass

		if res:
			token, result_class, results = res
			if result_class=='done':
				pass
			elif result_class=='running':
				pass
			elif result_class=='connected':
				pass
			elif result_class=='error':
				pass
			elif result_class=='exit':
				pass

	def scroll_to_end(self):
		iter = self.buf.get_end_iter()
		self.output_view.scroll_to_iter(iter, 0.0)
		return False

	def on_change_page(self, action, current):
		text = current.get_name()
		if text=='output':
			self.nb.set_current_page(0)
		else:
			self.nb.set_current_page(1)

	def on_setup(self):
		dlg = gtk.Dialog('setup', parent=self.main_window, buttons=(gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
		table = gtk.Table(rows=3, columns=2)
		table.set_row_spacings(2)
		table.attach(gtk.Label('target : '),            0, 1, 0, 1)
		table.attach(gtk.Label('args : '),              0, 1, 1, 2)
		table.attach(gtk.Label('working directory : '), 0, 1, 2, 3)

		entrys = gtk.Entry(), gtk.Entry(), gtk.Entry()
		table.attach(entrys[0], 1, 2, 0, 1)
		table.attach(entrys[1], 1, 2, 1, 2)
		table.attach(entrys[2], 1, 2, 2, 3)

		dlg.vbox.pack_start(table)
		
		if self.target:
			entrys[0].set_text(self.target)
		if self.args:
			entrys[1].set_text(self.args)
		if self.working_directory:
			entrys[2].set_text(self.working_directory)
		
		dlg.show_all()
		res = dlg.run()
		if res==gtk.RESPONSE_OK:
			text = entrys[0].get_text().strip()
			if len(text) > 0:
				self.target = text
			text = entrys[1].get_text().strip()
			if len(text) > 0:
				self.args = text
			text = entrys[2].get_text().strip()
			if len(text) > 0:
				self.working_directory = text
		dlg.destroy()

	def update_stack_list_locals(self):
		obs, res = self.stack_list_locals(1)
		locals = res[2]['locals']
		self.output_text('locals:\n')
		for var in locals:
			self.output_text('\t%s = %s\n' % (var['name'], var['value']))
			# update self.locals_view

	def on_dispatch_timer(self):
		try:
			self.dispatch(self.handle_recv)
		except Exception, e:
			print 'Error : ', e
		return True

	def on_cmd_entry_key_press(self, widget, event):
		if event.keyval==gtk.keysyms.Return:
			self.do_send_command()
		return False

	def do_send_command(self):
		cmd = self.cmd_entry.get_text().strip().lower()
		self.cmd_entry.set_text("")
		if cmd[:1]=='-':
			# gdbmi commands
			print self.call(cmd)
			
		else:
			# cli commands
			cmd = cmd.split()
			if len(cmd)==0:
				cmd = self.last_cmd
				if len(cmd)==0:
					return
			else:
				self.last_cmd = cmd

			self.do_lj_cli_command(cmd)

	def do_lj_cli_command(self, cmd):
		id = cmd[0]
		if id in ('n', 'next'):
			output = self.exec_next()
		elif id in ('s', 'step'):
			output = self.exec_step()
		elif id in ('si', 'stepi'):
			output = self.exec_step_instruction()
		elif id in ('c', 'continue'):
			output = self.exec_continue()
		elif id in ('b', 'break'):
			id, where = cmd
			output = self.break_insert(where)
		else:
			output = None
		print output

	def handle_debug_stopped(self, filename, line):
		self.output_text(str((filename, line)) + '\n')

NOTICE = """NOTICE :
    This plugin not finished! Just test now!
    1. used gdb/MI, for windows and linux.
    2. use python implements it.
        gdbmi package at puss/plugins/gdbmi, it's free also
    3. usage :
        [setup]     -> setup debug taget, startup args and working directory
        [prepare]   -> prepare gdb/MI environment
                       on Windows need gdb.exe in %PATH%
                       on Linux also need gdb
        [start/run]	-> 'start' will call gdb 'start' command and break at main
                    -> 'run' will call gdb'-exec-run' command
        [stop]      -> stop debug and close gdb
		cmdline		-> MI commands

"""

try:
	import puss

	class PussGdbPanel(GdbPanel):
		def __init__(self, *args, **kwargs):
			GdbPanel.__init__(self, *args, **kwargs)

		def handle_debug_stopped(self, filename, line):
			puss.doc_open(filename, line-1)

	gdb_panel = PussGdbPanel(puss.main_window)
	gdb_panel.output_text(NOTICE)

	def active():
		gdb_panel.show_all()
		
		puss.main_window.bottom_panel.append_page(gdb_panel, gtk.Label('Debug'))
		gdb_panel.connect('focus_in_event', lambda *args : gdb_panel.cmd_entry.grab_focus())

	def deactive():
		pass

except:
	pass

if __name__=='__main__':
	import sys
	p = GdbPanel()
	p.output_text(NOTICE)
	if sys.platform=='win32':
		p.target = 'd:/projects/a.exe'
	else:
		p.target = '/home/lj/a'
	w = gtk.Window()
	w.resize(800, 400)
	w.connect('destroy', gtk.main_quit)
	w.add(p)
	w.add_accel_group( p.uimgr.get_accel_group() )
	w.show_all()

	gtk.main()

