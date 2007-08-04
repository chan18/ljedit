#!/usr/bin/env python

import os, stat, time, sys
import pygtk
pygtk.require('2.0')
import gtk

folderxpm = [
	"17 16 7 1",
	"  c #000000",
	". c #808000",
	"X c yellow",
	"o c #808080",
	"O c #c0c0c0",
	"+ c white",
	"@ c None",
	"@@@@@@@@@@@@@@@@@",
	"@@@@@@@@@@@@@@@@@",
	"@@+XXXX.@@@@@@@@@",
	"@+OOOOOO.@@@@@@@@",
	"@+OXOXOXOXOXOXO. ",
	"@+XOXOXOXOXOXOX. ",
	"@+OXOXOXOXOXOXO. ",
	"@+XOXOXOXOXOXOX. ",
	"@+OXOXOXOXOXOXO. ",
	"@+XOXOXOXOXOXOX. ",
	"@+OXOXOXOXOXOXO. ",
	"@+XOXOXOXOXOXOX. ",
	"@+OOOOOOOOOOOOO. ",
	"@                ",
	"@@@@@@@@@@@@@@@@@",
	"@@@@@@@@@@@@@@@@@"
	]
folderpb = gtk.gdk.pixbuf_new_from_xpm_data(folderxpm)

filexpm = [
	"12 12 3 1",
	"  c #000000",
	". c #ffff04",
	"X c #b2c0dc",
	"X        XXX",
	"X ...... XXX",
	"X ......   X",
	"X .    ... X",
	"X ........ X",
	"X .   .... X",
	"X ........ X",
	"X .     .. X",
	"X ........ X",
	"X .     .. X",
	"X ........ X",
	"X          X"
	]
filepb = gtk.gdk.pixbuf_new_from_xpm_data(filexpm)

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

def fill_subs(model, parent_iter):
	row = model[parent_iter]
	if row[2]=='dir-expanded':
		iter = model.iter_children(parent_iter)
		while iter != None:
			row = model[iter]
			if row[2] == 'dir':
				row[2] = 'dir-expanded'
				pathname = row[1]
				for f in os.listdir(pathname):
					sp = os.path.join(pathname, f)
					sign = 'dir' if is_dir(sp) else 'file'
					model.append(iter, [f.decode(sys.getfilesystemencoding()), sp, sign])
			iter = model.iter_next(iter)
		
	elif row[2]=='dir':
		row[2] = 'dir-expanded'
		for f in os.listdir(pathname):
			sp = os.path.join(pathname, f)
			sign = 'dir-expanded' if is_dir(sp) else 'file'
			iter = model.append(parent_iter, [f.decode(sys.getfilesystemencoding()), sp, sign])
			if sign=='dir-expanded':
				for ssf in os.listdir(sp):
					ssp = os.path.join(sp, ssf)
					sign = 'dir' if is_dir(ssp) else 'file'
					model.append(iter, [ssf.decode(sys.getfilesystemencoding()), ssp, sign])
		
	elif row[2]=='win32root':
		row[2] = 'win32root-expanded'
		drivers = get_win32_drivers()
		for volume, driver in drivers:
			iter = model.append(parent_iter, [volume, driver, 'dir-expanded'])
			
			try:
				for f in os.listdir(driver):
					sp = os.path.join(driver, f)
					sign = 'dir' if is_dir(sp) else 'file'
					model.append(iter, [f.decode(sys.getfilesystemencoding()), sp, sign])
			except:
				pass

class FileExplorer(gtk.VBox):
	def __init__(self):
		gtk.VBox.__init__(self)
		
		self.treeview = self.make_file_view()
		model = self.make_root_model()
		self.treeview.set_model(model)
		
		self.scrolledwindow = gtk.ScrolledWindow()
		self.scrolledwindow.add(self.treeview)
		self.pack_start(self.scrolledwindow)
		self.set_size_request(200, 100)
		
 	def sort_filename_method(self, model, iter1, iter2):
 		if model[iter1][2]=='file':
 			if model[iter2][2]=='file':
 				return -1 if model[iter1][1] < model[iter2][1] else 1
 			return 1
 		else:
 			if model[iter2][2]=='file':
 				return -1
 			return -1 if model[iter1][1] < model[iter2][1] else 1
 		
	def make_root_model(self):
		model = gtk.TreeStore(str, str, str)	# display, path, [file, dir, dir-expanded]
		model.set_default_sort_func(self.sort_filename_method)
		pathname = '' if sys.platform=='win32' else '/'
		iter = model.append(None, ['my computer', pathname, 'win32root'])
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
			pb = filepb
		else:
			pb = folderpb
		cell.set_property('pixbuf', pb)
		
	def cb_file_name(self, column, cell, model, iter):
		cell.set_property('text', model.get_value(iter, 0))
		
	def cb_file_activated(self, filename):
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

	def deactive():
		left = ljedit.main_window.left_panel
		left.remove_page(explorer.page_id)

except:
	pass

