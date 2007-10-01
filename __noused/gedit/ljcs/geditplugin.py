# geditplugin.py
#

import gobject, gtk, gtksourceview
#import gnomevfs

from gtk import glade

import gedit
import os, os.path

GLADE_FILE = os.path.join(os.path.dirname(__file__), 'ljcs.glade')

slmgr = gtksourceview.SourceLanguagesManager()
cpplang = slmgr.get_language_from_mime_type('text/x-c++src')

import ljcs_py
import option

# init ljcs_py
# 
udm_file = os.path.join(os.path.dirname(__file__), 'user_defined_macros.h')
prefile = ljcs_py.env.pre_parse_files
prefile.clear()
prefile.push_back('/usr/include/sys/cdefs.h')
prefile.push_back(udm_file)

class WrapTask:
	def __init__(self):
		self.cbs = []
		self.task = ljcs_py.ParseTask()
		self.task.run()
		gobject.timeout_add(1000, self.query)

	def add(self, filename, cb):
		self.task.add(filename)
		self.cbs.append(cb)

	def query(self):
		if self.task.has_result():
			f = self.task.query()
			cb = self.cbs.pop(0)
			cb(f)
		return True

ljcs_py.parse_task = WrapTask()

class MatchedSet(ljcs_py.IMatched, set):
	def __init__(self, *args):
		ljcs_py.IMatched.__init__(self)
		set.__init__(self, *args)
		
	def on_matched(self, elem):
		self.add(elem)

ljcs_py.MatchedSet = MatchedSet

class DocIter(ljcs_py.IDocIter):
	def __init__(self, it):
		ljcs_py.IDocIter.__init__(self, str(it.get_char()))
		self.it = it

	def do_prev(self):
		if self.it.backward_char():
			return str(self.it.get_char())
		return '\0'

	def do_next(self):
		if self.it.forward_char():
			return str(self.it.get_char())
		return '\0'

def find_key(it):
	ps = DocIter(it.copy())
	pe = DocIter(it.copy())
	key = ljcs_py.py_find_key(ps, pe)
	if len(key)!=0:
		return key, ps.it, pe.it

def find_keys(it, sfile):
	# find key and macro replaced key
	# 
	result = find_key(it)
	if result:
		key, ps, pe = result
		keys = [key]
		
		# find macro replaced key
		rkey = ljcs_py.parse_macro_replace(str(ps.get_text(pe)), sfile)
		rkey = ljcs_py.parse_key(rkey)
		if len(rkey) > 0 and rkey != key:
			keys.append(rkey)
		return keys, ps, pe

def check_is_cpp_file(filename):
	for suffix in ('.cpp', '.hpp', '.c', '.h', '.cc'):
		if filename.endswith(suffix):
			return True
	for path in ljcs_py.env.include_paths:
		if filename.startswith(path):
			return True
	return False

import re
re_include = re.compile('^#[ \t]*include[ \t]*(.*)')
re_include_tip = re.compile('(["<])(.*)')
re_include_info = re.compile('(["<])([^">]*)[">].*')

class TipWindow:
	def __init__(self, parent):
		self.parent = parent
		self.words = None
		self.tag = None
		
		wtree =  glade.XML(GLADE_FILE, 'ljcs_tip_window')
		self.window = wtree.get_widget('ljcs_tip_window')
		self.window.set_transient_for(parent)
		
		self.store = gtk.ListStore(gobject.TYPE_STRING)
		self.view = wtree.get_widget('tv_tip')
		self.view.set_model(self.store)
		column = gtk.TreeViewColumn('', gtk.CellRendererText(), text=0)
		self.view.append_column(column)

	def get_selected(self):
		return self.view.get_selection().get_selected_rows()[1][0][0]

	def select_next(self):
		row = min(self.get_selected() + 1, len(self.store) - 1)
		selection = self.view.get_selection()
		selection.unselect_all()
		selection.select_path(row)
		self.view.scroll_to_cell(row)

	def select_previous(self):
		row = max(self.get_selected() - 1, 0)
		selection = self.view.get_selection()
		selection.unselect_all()
		selection.select_path(row)
		self.view.scroll_to_cell(row)

	def visible(self):
		return self.words!=None

	def show(self, x, y, words, tag):
		self.tag = tag
		
		words = set([ (w.upper(), w) for w in words ])	# union ignore case
		words = [w for w in words]						# sort
		words.sort()
		words = [ v for k, v in words ]					# words
		self.words = words

		#self.window.resize(1, 1)
		self.view.set_model(None)
		self.store.clear()
		for word in self.words:
			self.store.append([unicode(word)])
		self.view.set_model(self.store)
		#self.view.columns_autosize()
		self.view.get_selection().select_path(0)
		
		rx, ry = self.parent.get_position()
		self.window.move(rx + x + 24, ry + y + 44)
		self.window.show()

	def hide(self):
		self.window.hide()
		self.words = None

	def complete(self):
		doc = self.parent.get_active_document()
		finished = 0
		it = doc.get_iter_at_mark(doc.get_insert())
		while it.backward_char():
			ch = str(it.get_char())
			if ch.isalnum() or ch=='_':
				finished += 1
			else:
				break
		index = self.get_selected()
		doc.insert_at_cursor(self.words[index][finished:])
		self.hide()

class BottomWindow:
	def __init__(self, parent):
		self.parent = parent
		self.idle_task = None
		self.open_define_file_task = None
		
		callbacks = {
			'on_btn_next_clicked'			: self.on_btn_next_clicked,
			'on_btn_open_clicked'			: self.on_btn_open_clicked,
			'on_en_search_key_changed'		: self.on_en_search_key_changed,
			'on_btn_option_reset_clicked'	: self.on_btn_option_reset_clicked,
			'on_btn_option_apply_clicked'	: self.on_btn_option_apply_clicked,
			'on_btn_option_cancel_clicked'	: self.on_btn_option_cancel_clicked
		}
		
		wtree = glade.XML(GLADE_FILE, 'ljcs_bottom_panel')
		wtree.signal_autoconnect(callbacks)
		self.panel = wtree.get_widget('ljcs_bottom_panel')
		self.pages = wtree.get_widget('nb_pages')
		
		#define page
		self.btn_next = wtree.get_widget('btn_next')
		self.label_tip = wtree.get_widget('label_tip')
		
		self.tb_preview = gedit.Document()
		self.tb_preview.set_language(cpplang)
		
		self.tv_preview = gedit.View(self.tb_preview)
		self.tv_preview.set_editable(False)
		self.tv_preview.show()
		scrollwin_tv = wtree.get_widget('scrollwin_tv')
		scrollwin_tv.add(self.tv_preview)
		
		self.defines = None
		self.define_idx = 0
		self.last_preview_file = None

		#search page
		self.en_key = wtree.get_widget('en_search_key')
		self.en_filter = wtree.get_widget('en_search_filter')
		self.result_view = wtree.get_widget('tv_search_results')
		
		# add result view
		self.results = gtk.ListStore(str)
		self.result_view.set_model(self.results)
		column = gtk.TreeViewColumn('', gtk.CellRendererText(), text=0)
		self.result_view.append_column(column)

		#option page
		self.tv_option = wtree.get_widget('tv_option')
		self.load_option()
		self.apply_option()

		# add filters
		self.filters = gtk.ListStore(str, str, object)
		self.filters.append( ['all', None, None] )
		filter_path = os.path.join(os.path.dirname(__file__), 'filters')
		for filename in os.listdir(filter_path):
			filepath = os.path.join(filter_path, filename)
			row = [filename, filepath, None]
			self.filters.append( row )
			ljcs_py.parse_task.add(filepath, lambda result : self.on_filter_parsed(filename, result))
			
		self.en_filter.set_model(self.filters)
		#self.en_filter.set_text_column(0)
		self.en_filter.set_active(0)

	def apply_option(self):
		buf = self.tv_option.get_buffer()
		text = buf.get_start_iter().get_text(buf.get_end_iter())
		try:
			options = option.parse_text(text)
		except Exception, e:
			print 'ERROR : bad option format'
			return

		ljcs_include_paths = ljcs_py.env.include_paths
		ljcs_include_paths.clear()
		for path in options['include_paths']:
			path = str(path)
			if path[-1] != '/':
				path += '/'
			ljcs_include_paths.push_back(path)

		option.save(options)

	def load_option(self):
		try:
			options = option.load_user_option()
		except Exception, e:
			options = option.load_default_option()
		self.tv_option.get_buffer().set_text(options['text'])
		
	def on_btn_option_reset_clicked(self, button):
		options = option.load_default_option()
		self.tv_option.get_buffer().set_text(options['text'])
		self.apply_option()
		
	def on_btn_option_apply_clicked(self, button):
		self.apply_option()

	def on_btn_option_cancel_clicked(self, button):
		self.load_option()

	def on_filter_parsed(self, filename, result):
		for r in self.filters:
			if r[0]==filename:
				#print 'on_filter_parsed :', filename
				r[2] = result

	def on_btn_next_clicked(self, button):
		#print 'on_btn_next_clicked', button
		size = len(self.defines)
		if size > 1:
			self.define_idx += 1
			if self.define_idx >= size:
				self.define_idx = 0
			self.update_define_tip()

	def on_btn_open_clicked(self, button):
		elem = self.defines[self.define_idx]
		# print 'open_file', elem.file.filename
		try:
			# !!! use window.tab_add_xxxx
			#     but now, just simple do it
			# 
			os.stat(elem.file.filename)
			os.system('gedit ' + elem.file.filename)
		except:
			return
		
		# core dump!!!! shit!!!
		# 
		# self.parent.create_tab_from_uri(uri, encoding, line, False, True)
		if self.open_define_file_task==None:
			self.open_define_file_task = gobject.idle_add(self.open_define_file, elem.name, elem.file.filename, elem.sline)

	def open_define_file(self, name, filename, line):
		gobject.source_remove(self.open_define_file_task)
		self.open_define_file_task = None
		
		view = self.parent.get_active_view()
		doc = view.get_buffer()
		if doc.get_uri_for_display()==filename:
			doc.goto_line(line - 1)
			doc.set_search_text(name, 0)
			lineit = doc.get_iter_at_line(line)
			view.scroll_to_iter(lineit, 0.1)

	def on_en_search_key_changed(self, widget):
		key = self.en_key.get_text()
		key = ljcs_py.parse_key(key)
		filter_name, filter_file, sfile = self.filters[self.en_filter.get_active()]
		# print 'search :', key, filter_name, filter_file
		if filter_file==None:
			files = [ r[2] for r in self.filters ]
		else:
			files = [sfile]

		results = ljcs_py.MatchedSet()
		for f in files:
			if f:
				ljcs_py.search(key, results, f)

		self.result_view.set_model(None)
		self.results.clear()
		for e in results:
			self.results.append( [e.decl] )
		self.result_view.set_model(self.results)

	def update_define_tip(self):
		if self.defines==None:
			return
			
		elem = self.defines[self.define_idx]
		self.label_tip.set_label(str(elem.decl))
		self.btn_next.set_label( '%d/%d' % (self.define_idx+1, len(self.defines)) )

		if self.last_preview_file!=elem.file.filename:
			self.last_preview_file = elem.file.filename
			
			buf = self.tb_preview
			buf.delete(buf.get_start_iter(), buf.get_end_iter())
			it = buf.get_end_iter()
			
			for d in gedit.app_get_default().get_documents():
				if d.get_uri_for_display()==elem.file.filename:
					buf.insert( it, d.get_text(d.get_start_iter(), d.get_end_iter()) )
					break
			else:
				f = open(elem.file.filename, 'r')
				text = f.read()
				f.close()
				try:
					buf.insert(it, text.decode('utf8'), -1)
				except:
					try:
						buf.insert(it, text.decode('gb18030'), -1)
					except:
						try:
							buf.insert(it, text, -1)
						except:
							buf.insert(it, 'can not decode file!\n')

		self.pages.set_current_page(1)
		gedit_bottom = self.parent.get_bottom_panel()
		gedit_bottom.activate_item(self.panel)
		gedit_bottom.show()
		self.parent.get_active_view().grab_focus()

		if self.idle_task==None:
			self.idle_task = gobject.idle_add( lambda : self.scroll_defines_to_elem(elem) )

	def scroll_defines_to_elem(self, elem):
		line = elem.sline - 1
		self.tb_preview.goto_line(line)
		self.tb_preview.set_search_text(elem.name, 0)
		lineit = self.tv_preview.get_buffer().get_iter_at_line(line)
		self.tv_preview.scroll_to_iter(lineit, 0.1)
		
		if self.idle_task != None:
			gobject.source_remove(self.idle_task)
			self.idle_task = None

	def show_defines(self, elems):
		size = len(elems)
		if size>0:
			self.defines = elems
			self.define_idx = 0
			self.update_define_tip()

class ProjectWindow:
	def __init__(self, parent):
		self.parent = parent
		
		wtree = glade.XML(GLADE_FILE, 'ljcs_project_panel')
		self.panel = wtree.get_widget('ljcs_project_panel')

class OutlineWindow:
	def __init__(self, parent):
		self.parent = parent
		
		wtree = glade.XML(GLADE_FILE, 'ljcs_outline_panel')
		self.panel = wtree.get_widget('ljcs_outline_panel')

class SymbolWindow:
	def __init__(self, parent):
		self.parent = parent
		
		wtree = glade.XML(GLADE_FILE, 'ljcs_symbol_panel')
		self.panel = wtree.get_widget('ljcs_symbol_panel')

class LJCSUI:
	def __init__(self, owner):
		self.owner = owner
		self.tip = TipWindow(self.owner)
		self.bottom = BottomWindow(self.owner)
		self.project = ProjectWindow(self.owner)
		self.outline = ProjectWindow(self.owner)
		self.symbol = ProjectWindow(self.owner)

class LJCSOps:
	def __init__(self, ui):
		self.ui = ui
		self.snode = None
		self.is_cpp_file = False

	def active_plugin(self, doc, err, view):
		filename = doc.get_uri_for_display()
		self.is_cpp_file = False
		if not check_is_cpp_file(filename):
			return
		self.is_cpp_file = True
		self.parse_doc(doc)

	def on_parsed(self, result):
		self.snode = result
		#print 'parsed :', result.filename

	def parse_doc(self, doc):
		#text = doc.get_text(doc.get_start_iter(), doc.get_end_iter())
		filename = doc.get_uri_for_display()
		ljcs_py.parse_task.add(filename, self.on_parsed)

	def key_press_callback(self, view, event):
		if not self.is_cpp_file:
			return

		if self.ui.tip.visible():
			if event.state & (gtk.gdk.CONTROL_MASK | event.state & gtk.gdk.MOD1_MASK):
				self.ui.tip.hide()
				
			elif event.keyval in map(ord, ('(', '.', ':', '-')):
				if self.ui.tip.tag=='show_hint':
					self.ui.tip.complete()
				
			elif event.keyval in map(ord, ('"', '>', '/')):
				if self.ui.tip.tag=='show_include_hint':
					self.ui.tip.complete()
				
			else:
				#print '--------', key
				if self.ui.tip.tag!='info':
					key = gtk.gdk.keyval_name(event.keyval)
					if key in ('Tab', 'Return'):
						self.ui.tip.complete()
						doc = view.get_buffer()
						it = doc.get_iter_at_mark (doc.get_insert())
						self.show_hint(view, it, '(')
						return True
					if key=='Up':
						self.ui.tip.select_previous()
						return True
					if key=='Down':
						self.ui.tip.select_next()
						return True
					if key=='Escape':
						self.ui.tip.hide()
						return True
				else:
					self.ui.tip.hide()

	def key_release_callback(self, view, event):
		if not self.is_cpp_file:
			return

		key = gtk.gdk.keyval_name(event.keyval)
		if key in ('Tab', 'Return', 'Up', 'Down', 'Escape', 'Shift_L', 'Shift_R'):
			return

		doc = view.get_buffer()
		it = doc.get_iter_at_mark(doc.get_insert())
		
		# check include line
		s = it.copy()
		s.set_line_offset(0)
		text = s.get_text(it)
		result = re_include.match(text)
		if result:
			self.show_include_hint(view, it, result.group(1))
			return

		if event.string in ('.', '(', '<'):
			self.show_hint(view, it, event.string)
			
		elif event.string.isalpha() or event.string=='_':
			self.show_hint(view, it, event.string)
			
		elif event.string==':':
			s = it.copy()
			if s.backward_chars(2) and s.get_char()==':':
				self.show_hint(view, it, event.string)
			
		elif event.string=='>':
			s = it.copy()
			if s.backward_chars(2) and s.get_char()=='-':
				self.show_hint(view, it, event.string)
			
		elif self.ui.tip.visible():
			self.ui.tip.hide()

	def button_release_callback(self, view, event):
		if not self.is_cpp_file:
			return

		# test CTRL state
		if event.state & gtk.gdk.CONTROL_MASK:
			if self.snode==None:
				return
				
			if self.ui.tip.visible():
				self.ui.tip.hide()

			doc = view.get_buffer()
			it = doc.get_iter_at_mark(doc.get_insert())
			s = it.copy()
			s.set_line_offset(0)
			e = it.copy()
			e.forward_to_line_end()
			text = s.get_text(e)
			result = re_include.match(text)
			if result:
				# open include file
				result = re_include_info.match(result.group(1))
				if result:
					self.open_include_file(doc, result.group(1), result.group(2))
			else:
				ch = it.get_char()
				if ch.isalnum() or ch=='_':
					while it.forward_word_end():
						ch = it.get_char()
						if ch=='_':
							continue
						break
					
				try:
					_keys, ps, pe = find_keys(it, self.snode)
					keys = []
					for key in _keys:
						keys.append(key.replace(':S', ':*'))
				except Exception, e:
					return

				line = it.get_line() + 1
				results = ljcs_py.MatchedSet()

				ljcs_py.search_keys(keys, results, self.snode, line)
				if len(results) > 0:
					elems = [e for e in results]
					self.ui.bottom.show_defines(elems)

	def motion_notify_callback(self, view, event):
		pass

	def open_include_file(self, doc, sign, filename):
		if sign=='<':
			paths = ljcs_py.env.include_paths
		else:
			paths = [doc.get_uri_for_display().rsplit('/', 1)[0] + '/']

		for spath in paths:
			spath = spath + filename
			try:
				os.stat(spath)
				os.system('gedit ' + spath)
			except:
				pass

	def show_include_hint(self, view, it, text):
		result = re_include_tip.match(text)
		if result==None:
			return
		sign, text = result.group(1), result.group(2)
		if sign=='<':
			paths = ljcs_py.env.include_paths
		else:
			paths = [view.get_buffer().get_uri_for_display().rsplit('/', 1)[0] + '/']

		hints = []
		for spath in paths:
			spath = spath + text
			try:
				path, key = spath.rsplit('/', 1)
				#print 'spath :', path, key
				for i in os.listdir(path):
					if i.startswith(key):
						hints.append(i)
			except:
				pass

		if len(hints) > 0:
			hints.sort()
			rect = view.get_iter_location(it)
			x, y = view.buffer_to_window_coords(gtk.TEXT_WINDOW_TEXT, rect.x, rect.y)
			x, y = view.translate_coordinates(self.ui.owner, x, y)
			self.ui.tip.show(x, y, hints, 'show_include_hint')
		else:
			self.ui.tip.hide()

		#print 'hints :', hints

	def show_hint(self, view, it, tag):
		#calc tip window size
		rect = view.get_iter_location(it)
		x, y = view.buffer_to_window_coords(gtk.TEXT_WINDOW_TEXT, rect.x, rect.y)
		x, y = view.translate_coordinates(self.ui.owner, x, y)

		if self.snode==None:
			self.ui.tip.show(x, y, ['warn : file in parsing'], 'info')
			return

		#print 'kkkkk :', get_cur_line(doc, it)
		try:
			keys, ps, pe = find_keys(it, self.snode)
		except Exception, e:
			self.ui.tip.show(x, y, ['warn : not find key!'], 'info')
			return

		line = it.get_line() + 1
		results = ljcs_py.MatchedSet()

		#print 'find', keys, ' in %s ----- %d' % (self.snode.filename, line)
		ljcs_py.search_keys(keys, results, self.snode, line)
		if len(results) > 0:
			if tag in ('(', '<'):
				self.ui.tip.hide()
				elems = [e for e in results]
				self.ui.bottom.show_defines(elems)
			else:
				#print 'show :', [e.decl for e in elems]
				# show ignore case union sort words
				words = [e.name for e in results]
				self.ui.tip.show(x, y, words, 'show_hint')

		else:
			self.ui.tip.hide()
			
			#text = ps.get_text(pe)
			#self.ui.tip.show(x, y, ['warn : no tip when find tag : %s' % text], 'info')

