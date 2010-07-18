#!/usr/bin/env python

import os, stat, time, sys
import pygtk
pygtk.require('2.0')
import gobject, gtk

import gettext

TEXT_DOMAIN = 'puss_py_ext_explorer'

_ = lambda s : gettext.dgettext(TEXT_DOMAIN, s)

gettext.bindtextdomain(TEXT_DOMAIN, os.path.join(os.path.dirname(__file__), '../locale'))
gettext.bind_textdomain_codeset(TEXT_DOMAIN, 'UTF-8')

def get_win32_drivers():
	drivers = []
	if sys.platform=='win32':
		#paths.append( ['home', os.path.expanduser('~\\')] )
		
		import ctypes
		DRIVER_TYPES = 'unknown', 'no_root_dir', 'removeable', 'localdisk', 'remotedisk', 'cdrom', 'ramdisk'
		
		sz = 512
		buf = ctypes.create_string_buffer(sz)
		
		ds = ctypes.windll.kernel32.GetLogicalDrives()
		for i in range(26):
			if (ds >> i) & 0x01:
				driver = chr(i+65) + ':\\'
				
				driver_type = ctypes.windll.kernel32.GetDriveTypeA(driver)
				
				if ctypes.windll.kernel32.GetVolumeInformationA(driver, buf, sz, None, None, None, None, 0):
					volume = buf.value
				else:
					volume = ''
				
				if len(volume)==0:			  
					if driver_type > len(DRIVER_TYPES):
						driver_type = 0
					volume = DRIVER_TYPES[driver_type]
				
				volume += '(%s)' % driver

				drivers.append( [volume, driver] )
	return drivers

def is_dir(pathname):
	try:
		filestat = os.stat(pathname)
		if stat.S_ISDIR(filestat.st_mode):
			return True
	except Exception:
		pass
	return False

def sort_file(a, b, path):
	sa = is_dir(os.path.join(path, a))
	sb = is_dir(os.path.join(path, b))
	if sa==sb:
		return 1 if a.lower() > b.lower() else -1
	else:
		return 1 if sb else -1

def fill_folder(model, parent_iter, pathname):
	try:
		fs = os.listdir(pathname)
		fs.sort(lambda a, b : sort_file(a, b, pathname))
		for f in fs:
			if f.startswith('.'):
				continue
			sp = os.path.join(pathname, f)
			sign = 'dir' if is_dir(sp) else 'file'
			iter = model.append(parent_iter, [f.decode(sys.getfilesystemencoding()), sp, sign, f])
			if sign=='dir':
				model.append(iter, ['loading...', 'loading...', 'loading...', 'loading...'])
		
	except Exception, e:
		pass

	try:
		# remove 'loading...' child
		iter = model.iter_children(parent_iter)
		if model[iter][2]=='loading...':
			del model[iter]
	except Exception, e:
		pass

def fill_subs(model, parent_iter):
	row = model[parent_iter]
	if row[2]=='dir':
		row[2] = 'dir-expanded'
		fill_folder(model, parent_iter, row[1])
		#iter = model.iter_children(parent_iter)
		#while iter != None:
		#   print model[iter][1]
		#   iter = model.iter_next(iter)
		
	elif row[2]=='win32root':
		row[2] = 'win32root-expanded'
		drivers = get_win32_drivers()
		for volume, driver in drivers:
			iter = model.append(parent_iter, [volume.decode(sys.getfilesystemencoding()), driver, 'dir', driver.lower()[:2]])
			model.append(iter, ['loading...', 'loading...', 'loading...', 'loading...'])

def locate_to(model, it, paths):
	if len(paths)==0:
		return it

	name = paths.pop(0)
	if len(name)==0:
		return locate_to(model, it, paths)

	sit = model.iter_children(it)
	while sit != None:
		row = model[sit]
		#print 'vvv :', row[3].lower(), name.lower()
		if sys.platform=='win32':
			if row[3].lower()==name.lower():
				fill_subs(model, sit)
				#print '-->', name
				return locate_to(model, sit, paths)
			
		else:
			if row[3]==name:
				fill_subs(model, sit)
				#print '-->', name
				return locate_to(model, sit, paths)
		sit = model.iter_next(sit)

class FileExplorer(gtk.VBox):
	def __init__(self):
		gtk.VBox.__init__(self)
		
		self.folderpb = self.render_icon(gtk.STOCK_DIRECTORY, gtk.ICON_SIZE_MENU)
		self.filepb   = self.render_icon(gtk.STOCK_FILE, gtk.ICON_SIZE_MENU)
		
		img = gtk.Image()
		img.set_from_stock(gtk.STOCK_REFRESH, gtk.ICON_SIZE_MENU)
		refreshbutton = gtk.Button()
		refreshbutton.set_image(img)
		refreshbutton.connect('clicked', self.on_refresh_clicked)
		
		self.fchooser = gtk.FileChooserButton('choose folder')
		self.fchooser.set_action(gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER)
		self.fchooser.set_border_width(3)
		self.fchooser.connect('current-folder-changed', self.on_folder_changed)
		
		hbox = gtk.HBox()
		hbox.pack_start(refreshbutton, False, True)
		hbox.pack_start(self.fchooser, True, True)
		self.pack_start(hbox, False, True)
		
		self.treeview = self.make_file_view()
		self.treeview.set_headers_visible(False)
		self.treeview.set_enable_tree_lines(True)
		self.refresh()
		self.on_folder_changed(self.fchooser)
		
		self.scrolledwindow = gtk.ScrolledWindow()
		self.scrolledwindow.add(self.treeview)
		self.pack_start(self.scrolledwindow)
		self.set_size_request(200, 100)
		
	def refresh(self):
		model = self.make_root_model()
		self.treeview.set_model(model)
		
		# export first layer
		rit = model.get_iter_root()
		path = model.get_path(rit)
		self.treeview.expand_to_path(path)
		
	def on_refresh_clicked(self, btn):
		m, i = self.treeview.get_selection().get_selected()
		if m==None or i==None:
			self.refresh()
			self.on_folder_changed(self.fchooser)
		else:
			filename = m[i][1]
			self.refresh()
			self.locate_to_file(filename)
		
	def on_folder_changed(self, fchooser):
		folder = fchooser.get_current_folder()
		self.locate_to_file(folder)
		
	def locate_to_file(self, filename):
		if sys.platform=='win32':
			filename = filename.replace('\\', '/')
		model = self.treeview.get_model()
		it = locate_to(model, model.get_iter_root(), filename.split('/'))
		#print 'get it : ', it
		if it:
			self.treeview.get_selection().select_iter(it)
			path = model.get_path(it)
			self.treeview.expand_to_path(path)
			#self.treeview.scroll_to_cell(path, None, True, 0.5, 0.0)
			gobject.idle_add(self.treeview.scroll_to_cell, path)
		
	def make_root_model(self):
		model = gtk.TreeStore(str, str, str, str)   # display, path, [file, dir, dir-expanded, name]
		if sys.platform=='win32':
			iter = model.append(None, [_('my computer'), u'', 'win32root', ''])
		else:
			iter = model.append(None, ['/', u'/', 'dir', ''])
		fill_subs(model, iter)
		return model
		
	def make_file_view(self):
		treeview = gtk.TreeView()
		cell_icon = gtk.CellRendererPixbuf()
		cell_text = gtk.CellRendererText()
		
		# icon, filename
		col = gtk.TreeViewColumn('filename')
		col.pack_start(cell_icon, False)
		col.set_cell_data_func(cell_icon, self.cb_file_icon)
		col.pack_start(cell_text, False)
		col.set_cell_data_func(cell_text, self.cb_file_name)
		treeview.append_column(col)
		
		treeview.connect('row-activated', self.cb_row_activated)
		treeview.connect('row-expanded',  self.cb_row_expanded)
		treeview.connect('key-press-event', self.cb_key_press)
		return treeview
		
	def cb_file_icon(self, column, cell, model, iter):
		sign = model[iter][2]
		if sign=='file':
			pb = self.filepb
		else:
			pb = self.folderpb
		cell.set_property('pixbuf', pb)
		
	def cb_file_name(self, column, cell, model, iter):
		cell.set_property('text', model.get_value(iter, 0))
		
	def cb_file_activated(self, filename):
		# test
		#self.locate_to_file('d:/mingw32/include/c++/3.4.2/bits/stl_list.h')
		print filename
		
	def cb_row_expanded(self, treeview, iter, path):
		model = treeview.get_model()
		iter = model.get_iter(path)
		fill_subs(model, iter)
		
	def cb_row_activated(self, treeview, path, column):
		model = treeview.get_model()
		iter = model.get_iter(path)
		if model[iter][2]=='file':
			self.cb_file_activated(model[iter][1])
		else:
			if treeview.row_expanded(path):
				treeview.collapse_row(path)
			else:
				treeview.expand_to_path(path)

	def cb_key_press(self, treeview, event):
		if event.keyval==gtk.keysyms.Left:
			gtk.bindings_activate(treeview, gtk.keysyms.BackSpace, 0)
			treeview.emit_stop_by_name('key-press-event')
			
		elif event.keyval==gtk.keysyms.Right:
			gtk.bindings_activate(treeview, gtk.keysyms.Return, 0)
			treeview.emit_stop_by_name('key-press-event')

if __name__ == "__main__":
	w = gtk.Window()
	w.connect("destroy", gtk.main_quit)
	
	v = FileExplorer()
	w.add(v)
	w.set_size_request(800, 600)
	w.show_all()
	
	gtk.main()

try:
	import puss

	class PussExplorer(FileExplorer):
		def __init__(self):
			FileExplorer.__init__(self)

		def cb_file_activated(self, filename):
			ufilename = filename.decode(sys.getfilesystemencoding()).encode('utf8')
			puss.doc_open(ufilename, -1, -1, True)

	def on_switch_page(nb, gp, page):
		buf = puss.doc_get_buffer_from_page_num(page)
		if buf==None:
			return

		filepath = puss.doc_get_url(buf)
		if filepath:
			ufilepath = filepath.decode('utf8')
			filepath = ufilepath.encode(sys.getfilesystemencoding())
			explorer.locate_to_file(filepath)

	def puss_plugin_active():
		global explorer
		explorer = PussExplorer()

		explorer.show_all()
		puss.panel_append(explorer, gtk.Label(_('Explorer')), "py_explorer_plugin_panel", puss.PANEL_POS_LEFT)
		doc_panel = puss.ui.get_object('doc_panel')
		explorer.switch_page_id = doc_panel.connect('switch-page', on_switch_page)

	def puss_plugin_deactive():
		global explorer
		doc_panel = puss.ui.get_object('doc_panel')
		doc_panel.disconnect(explorer.switch_page_id)
		puss.panel_remove(explorer)
		explorer = None

except Exception:
	pass

