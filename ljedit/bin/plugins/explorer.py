#!/usr/bin/env python

import os, stat, time, sys
import pygtk
pygtk.require('2.0')
import gobject, gtk

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
	except:
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
			sp = os.path.join(pathname, f)
			sign = 'dir' if is_dir(sp) else 'file'
			model.append(parent_iter, [f.decode(sys.getfilesystemencoding()), sp, sign, f])
	except:
		pass

def fill_subs(model, parent_iter):
	row = model[parent_iter]
	if row[2]=='dir-expanded':
		iter = model.iter_children(parent_iter)
		while iter != None:
			row = model[iter]
			if row[2] == 'dir':
				row[2] = 'dir-expanded'
				fill_folder(model, iter, row[1])
			iter = model.iter_next(iter)
		
	elif row[2]=='dir':
		row[2] = 'dir-expanded'
		fill_folder(model, parent_iter, row[1])
		#iter = model.iter_children(parent_iter)
		#while iter != None:
		#	print model[iter][1]
		#	iter = model.iter_next(iter)
		
	elif row[2]=='win32root':
		row[2] = 'win32root-expanded'
		drivers = get_win32_drivers()
		for volume, driver in drivers:
			iter = model.append(parent_iter, [volume, driver, 'dir-expanded', driver.lower()[:2]])
			fill_folder(model, iter, driver)

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
		
		self.fchooser = gtk.FileChooserButton('choose folder')
		self.fchooser.set_action(gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER)
		self.fchooser.set_border_width(3)
		self.fchooser.connect('current-folder-changed', self.on_folder_changed)
		self.pack_start(self.fchooser, False, True)
		
		self.treeview = self.make_file_view()
		self.treeview.set_headers_visible(False)
		model = self.make_root_model()
		self.treeview.set_model(model)
		
		self.scrolledwindow = gtk.ScrolledWindow()
		self.scrolledwindow.add(self.treeview)
		self.pack_start(self.scrolledwindow)
		self.set_size_request(200, 100)
		
		# export first layer
		rit = model.get_iter_root()
		path = model.get_path(rit)
		self.treeview.expand_to_path(path)
		
		self.on_folder_changed(self.fchooser)
		
	def on_folder_changed(self, fchooser):
		folder = fchooser.get_current_folder()
		if sys.platform=='win32':
			folder = folder.replace('\\', '/')
		self.locate_to_file(folder)
		
	def locate_to_file(self, filename):
		model = self.treeview.get_model()
		it = locate_to(model, model.get_iter_root(), filename.split('/'))
		#print 'get it : ', it
		if it:
			self.treeview.get_selection().select_iter(it)
			path = model.get_path(it)
			self.treeview.expand_to_path(path)
			self.treeview.scroll_to_cell(path)
			gobject.idle_add(self.treeview.scroll_to_cell, path)
		
	def make_root_model(self):
		model = gtk.TreeStore(str, str, str, str)	# display, path, [file, dir, dir-expanded, name]
		if sys.platform=='win32':
			iter = model.append(None, ['my computer', '', 'win32root', ''])
		else:
			iter = model.append(None, ['/', '/', 'dir', ''])
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

if __name__ == "__main__":
	w = gtk.Window()
	w.connect("destroy", gtk.main_quit)
	
	v = FileExplorer()
	w.add(v)
	w.set_size_request(800, 600)
	w.show_all()
	
	gtk.main()

try:
	import ljedit

	class LJEditExplorer(FileExplorer):
		def __init__(self):
			FileExplorer.__init__(self)
			
		def cb_file_activated(self, filename):
			ljedit.main_window.doc_manager.open_file(filename)

	explorer = LJEditExplorer()

	def active():
		explorer.show_all()

		left = ljedit.main_window.left_panel
		explorer.page_id = left.append_page(explorer, gtk.Label('Explorer'))
		
		def on_switch_page(nb, gp, page):
			filepath = ljedit.main_window.doc_manager.get_file_path(page)
			explorer.locate_to_file(filepath)
			
		explorer.switch_page_id = ljedit.main_window.doc_manager.connect('switch-page', on_switch_page)

	def deactive():
		ljedit.main_window.doc_manager.disconnect(explorer.switch_page_id)
		left = ljedit.main_window.left_panel
		left.remove_page(explorer.page_id)

except:
	pass

