﻿# ljed_debug.py
# 
import gdbmi

import gobject, gtk

try:
	import ljedit

except:
	class ljedit:
		main_window = None

def append_button(vbox, title, clickcb):
	btn = gtk.Button(title)
	btn.connect('clicked', clickcb)
	vbox.pack_start(btn)

class GdbPanel(gtk.HBox, gdbmi.Driver):
	def __init__(self):
		gtk.HBox.__init__(self)

		gdbmi.Driver.__init__(self)
		self.last_cmd = ()

		vbox = gtk.VBox()
		append_button(vbox, 'setup',    lambda *args : self.on_setup())
		append_button(vbox, 'prepare',  lambda *args : self.prepare())
		append_button(vbox, 'start',    lambda *args : self.start())
		append_button(vbox, 'run',      lambda *args : self.run())
		append_button(vbox, 'stop',     lambda *args : self.stop())
		append_button(vbox, 'next',     lambda *args : self.call('-exec-next'))
		append_button(vbox, 'step',     lambda *args : self.call('-exec-step'))
		append_button(vbox, 'continue', lambda *args : self.call('-exec-continue'))
		append_button(vbox, 'stack__list__locals',  lambda *args : self.on_stack_list_locals())

		self.cmd_entry = gtk.Entry()
		self.cmd_entry.connect('key_press_event', self.on_cmd_entry_key_press)
		vbox.pack_start(self.cmd_entry)

		self.pack_start(vbox, False)

		self.buf = gtk.TextBuffer()
		self.command_tag = self.buf.create_tag(foreground='red')
		self.packet_tag = self.buf.create_tag(foreground='gray')
		self.output_tag = self.buf.create_tag(foreground='blue')
		self.info_tag = self.buf.create_tag(foreground='green')

		self.view = gtk.TextView(self.buf)
		sw = gtk.ScrolledWindow()
		sw.add(self.view)
		sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		self.pack_start(sw)

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
		self.view.scroll_to_iter(iter, 0.0)
		return False

	def on_setup(self):
		dlg = gtk.Dialog('setup', parent=ljedit.main_window, buttons=(gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
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

	def on_stack_list_locals(self):
		obs, res = self.stack_list_locals(1)
		locals = res[2]['locals']
		self.output_text('locals:\n')
		for var in locals:
			self.output_text('\t%s = %s\n' % (var['name'], var['value']))

	def on_dispatch_timer(self):
		try:
			self.dispatch()
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
        gdbmi package at ljedit/plugins/gdbmi, it's free also
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
	class LJED_GdbPanel(GdbPanel):
		def __init__(self, *args, **kwargs):
			GdbPanel.__init__(self, *args, **kwargs)

		def handle_debug_stopped(self, filename, line):
			ljedit.main_window.doc_manager.open_file(filename, line-1)

	gdb_panel = LJED_GdbPanel()
	gdb_panel.output_text(NOTICE)

	def active():
		gdb_panel.show_all()
		
		bottom = ljedit.main_window.bottom_panel
		gdb_panel.page_id = bottom.append_page(gdb_panel, gtk.Label('Debug'))
		gdb_panel.__ljedit_active_id = gdb_panel.connect('focus_in_event', lambda *args : gdb_panel.cmd_entry.grab_focus())

	def deactive():
		bottom = ljedit.main_window.bottom_panel
		gdb_panel.disconnect(gdb_panel.__ljedit_active_id)
		bottom.remove_page(gdb_panel.page_id)

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
	w.show_all()

	gtk.main()

