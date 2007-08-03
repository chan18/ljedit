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
 		
	def make_root_model(self):
		paths = []
		if sys.platform=='win32':
			paths.append( ['home', os.path.expanduser('~\\')] )
			
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

					paths.append( [volume, driver] )
			
			
		else:
			paths.append( ['home', os.path.expanduser('~/')] )
			paths.append( ['/', '/'] )
		
		
		model = gtk.TreeStore(str, str)
		for path in paths:
			model.append(None, path)
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
		return treeview
		
	def cb_file_icon(self, column, cell, model, iter):
		filename = model.get_value(iter, 1)
		try:
			filestat = os.stat(filename)
			if stat.S_ISDIR(filestat.st_mode):
				pb = folderpb
			else:
				pb = filepb
			cell.set_property('pixbuf', pb)
		except:
			pass
		
	def cb_file_name(self, column, cell, model, iter):
		cell.set_property('text', model.get_value(iter, 0))
		
	def cb_file_activated(self, filename):
		print filename
		
	def cb_row_activated(self, treeview, path, column):
		model = treeview.get_model()
		iter = model.get_iter(path)
		filename = model.get_value(iter, 1)
		try:
			filestat = os.stat(filename)
		except:
			return

		if stat.S_ISDIR(filestat.st_mode):
			for path in os.listdir(filename):
				model.append( iter, [path.decode(sys.getfilesystemencoding()), os.path.join(filename, path)] )
		else:
			self.cb_file_activated(filename)

if __name__ == "__main__":
	w = gtk.Window()
	v = FileExplorer()
	w.add(v)
	w.show_all()
	gtk.main()

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


