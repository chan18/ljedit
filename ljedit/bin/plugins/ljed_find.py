# ljed_find.py
# 

import ljedit

import os
import gtk
from gtk import glade

GLADE_FILE = os.path.join(os.path.dirname(__file__), 'find.glade')

UI_INFO = """	<ui>
					<menubar name='MenuBar'>
						<menu action='EditMenu'>
							<menuitem action='FindReplace'/>
						</menu>
					</menubar>
				</ui>
"""

def find_in_file(results, find_text, filepath):
	try:
		f = open(filepath, 'r')
		lines = f.readlines()
		f.close()
	except Exception:
		return
		
	for line, text in enumerate(lines):
		if find_text in text:
			results.append([filepath, line+1, text.strip(), 0])

def find_in_opened_file(results, find_text, page_num):
	if page_num >= 0:
		filepath = ljedit.main_window.doc_manager.get_file_path(page_num)
		fileview = ljedit.main_window.doc_manager.get_text_view(page_num)
		filebuf = fileview.get_buffer()
		
		it = filebuf.get_start_iter()
		while not it.is_end():
			ps, pe = it.forward_search(find_text, gtk.TEXT_SEARCH_TEXT_ONLY)
			offset = ps.get_line_offset()
			ps.set_line_offset(0)
			pe.set_line_offset(pe.get_chars_in_line())
			text = ps.get_text(pe).strip()
			pos = [filepath, ps.get_line()+1, text, offset]
			#print pos
			results.append(pos)
			it = pe

class FindWindow:
	def __init__(self):
		# panel
		callbacks = {
			  'on_FindPanel_FindNextButton_clicked'		: self.on_FindNextButton_clicked
			, 'on_FindPanel_ResultView_row_activated'	: self.on_ResultView_row_activated
			, 'on_FindPanel_FindText_key_press_event'	: self.on_FindText_key_press_event
		}
		
		wtree = glade.XML(GLADE_FILE, 'FindPanel')
		wtree.signal_autoconnect(callbacks)
		
		self.panel = wtree.get_widget('FindPanel')
		
		self.find_text_entry = wtree.get_widget('FindPanel_FindText')
		self.range_cur_file = wtree.get_widget('FindPanel_RangeCurFile')
		self.range_cur_file_dir = wtree.get_widget('FindPanel_RangeCurFileDir')
		self.range_opened_files = wtree.get_widget('FindPanel_RangeOpenedFiles')
		
		self.result_model = gtk.ListStore(str, int, str, int)   # [file, line, text, offset]
		self.result_view = wtree.get_widget('FindPanel_ResultView')
		self.result_view.set_model(self.result_model)
		
		render = gtk.CellRendererText()
		self.result_view.append_column(gtk.TreeViewColumn('file', render, text=0))
		self.result_view.append_column(gtk.TreeViewColumn('line', render, text=1))
		self.result_view.append_column(gtk.TreeViewColumn('text', render, text=2))
		self.result_view.connect('row-activated', self.on_ResultView_row_activated)
		
		# UI
		action_group = gtk.ActionGroup('FindAction')
		action = gtk.Action('FindReplace', 'Find_Replace', 'find/replace text', gtk.STOCK_QUIT)
		action.connect('activate', self.on_FindPanel_activate)
		action_group.add_action_with_accel(action, '<CONTROL>F')
		
		ui_manager = ljedit.main_window.ui_manager
		ui_manager.insert_action_group(action_group, 0)
		self.menu_id = ui_manager.add_ui_from_string(UI_INFO)
		ui_manager.ensure_update()
		
	def free(self):
		ljedit.main_window.ui_manager.remove_ui(self.menu_id)
		
	def set_results(self, results):
		self.result_view.set_model(None)
		self.result_model.clear()
		for r in results:
			self.result_model.append(r)
		self.result_view.set_model(self.result_model)
		
	def do_find(self):
		dm = ljedit.main_window.doc_manager
		find_text = self.find_text_entry.get_active_text()
		if len(find_text)==0:
			return
		else:
			s = self.find_text_entry.get_model()
			if s==None:
				s = gtk.ListStore(str)
				self.find_text_entry.set_model(s)
			for i, in s:
				if i==find_text:
					break
			else:
				self.find_text_entry.append_text(find_text)
		
		results = []
		
		if self.range_cur_file.get_active():
			try:
				find_in_opened_file(results, find_text, dm.get_current_page())
			except Exception, e:
				pass
			
		elif self.range_cur_file_dir.get_active():
			page_num = dm.get_current_page()
			if page_num<0:
				return
			
			filepath = ljedit.main_window.doc_manager.get_file_path(page_num)
			for r, ds, fs in os.walk(os.path.dirname(filepath)):
				for f in fs:
					if f.endswith( ('.cpp', '.hpp', '.cc', '.hh', '.c', '.h', '.py') ):
						try:
							find_in_file(results, find_text, os.path.join(r, f))
						except Exception, e:
							pass
			
		elif self.range_opened_files.get_active():
			for i in range(len(dm.get_children())):
				try:
					find_in_opened_file(results, find_text, i)
				except Exception, e:
					pass
			
		self.set_results(results)
		
	def on_FindPanel_activate(self, action):
		ljedit.main_window.bottom_panel.set_current_page(find_window.page_id)
		self.find_text_entry.grab_focus()
		
	def on_FindText_key_press_event(self, *args):
		print args
		self.do_find()
		
	def on_FindNextButton_clicked(self, widget):
		self.do_find()
		
	def on_ResultView_row_activated(self, treeview, path, column):
		iter = self.result_model.get_iter(path)
		file, line, text, offset = self.result_model[iter]
		ljedit.main_window.doc_manager.open_file(file, line-1)

find_window = FindWindow()

def active():
	bottom = ljedit.main_window.bottom_panel
	find_window.page_id = bottom.append_page(find_window.panel, gtk.Label('Find'))

def deactive():
	find_window.free()
	bottom = ljedit.main_window.bottom_panel
	bottom.remove_page(find_window.page_id)

