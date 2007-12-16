# options.py
# 
import ljedit
import gtk

UI_INFO = """	<ui>
					<menubar name='MenuBar'>
						<menu action='ToolsMenu'>
							<menuitem action='Options'/>
						</menu>
					</menubar>
				</ui>
"""

class OptionsManager:
	def __init__(self):
		pass

	def create(self):
		# UI
		action_group = gtk.ActionGroup('OptionAction')
		action = gtk.Action('Options', '_Options', 'setup options', gtk.STOCK_STOP)
		action.connect('activate', self.on_Menu_Option_activate)
		action_group.add_action(action)

		ui_manager = ljedit.main_window.ui_manager
		ui_manager.insert_action_group(action_group, 0)
		self.menu_id = ui_manager.add_ui_from_string(UI_INFO)
		ui_manager.ensure_update()

	def destroy(self):
		ljedit.main_window.ui_manager.remove_ui(self.menu_id)

	def on_Menu_Option_activate(self, action):
		dlg = self.create_option_dialog()
		if dlg.run()==gtk.RESPONSE_OK:
			ljedit.config_manager.save_options()
			
			for id, (old, new)  in dlg.changed_options.items():
				ljedit.config_manager.option_changed_callback(id, new, old)
				
		dlg.destroy()

	def create_option_dialog(self):
		model = gtk.ListStore(object)
		
		ops = [option for option in ljedit.config_manager.options.values()]
		ops.sort( lambda a, b : cmp(a.id, b.id) )
		for option in ops:
			if hasattr(option, 'data_type'):
				model.append([option])
		
		view = gtk.TreeView(model)
		view.set_grid_lines(gtk.TREE_VIEW_GRID_LINES_HORIZONTAL)
		
		render = gtk.CellRendererText()
		blue_render = gtk.CellRendererText()
		blue_render.set_property('foreground', 'blue')
		
		col = gtk.TreeViewColumn('id', render)
		col.set_cell_data_func(render, self.cb_render_option, 'id')
		view.append_column(col)
		col = gtk.TreeViewColumn('type', blue_render)
		col.set_cell_data_func(blue_render, self.cb_render_option, 'data_type')
		view.append_column(col)
		col = gtk.TreeViewColumn('value', render)
		col.set_cell_data_func(render, self.cb_render_option, 'value')
		view.append_column(col)
		col = gtk.TreeViewColumn('tip', blue_render)
		col.set_cell_data_func(blue_render, self.cb_render_option, 'tip')
		view.append_column(col)
		
		sw = gtk.ScrolledWindow()
		sw.add(view)
		sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		sw.show_all()
		
		dlg = gtk.Dialog(parent=ljedit.main_window, buttons=('ok', gtk.RESPONSE_OK, 'cancel', gtk.RESPONSE_CANCEL))
		dlg.vbox.add(sw)
		dlg.resize(800, 600)
		view.connect('row-activated', lambda *args : self.on_Option_row_actived(dlg, *args))
		
		dlg.changed_options = {}
		
		return dlg

	def cb_render_option(self, col, cell, model, iter, attr):
		option = model.get_value(iter, 0)
		if option.value==option.default_value:
			markup = getattr(option, attr)
		else:
			markup = '<b>'+getattr(option, attr)+'</b>'
		cell.set_property('markup', markup)

	def make_modify_option_dialog(self, option_dialog, option):
		dlg = gtk.Dialog(parent=option_dialog, buttons=('default', gtk.RESPONSE_REJECT, 'ok', gtk.RESPONSE_OK, 'cancel', gtk.RESPONSE_CANCEL))
		if option.data_type=='font':
			fs = gtk.FontSelection()
			fs.set_preview_text( 'welcome to use ljedit!' )
			dlg.vbox.add(fs)
			fs.set_font_name(option.value)
			dlg.ljedit_get_option = fs.get_font_name
			
		elif option.data_type=='text':
			buf = gtk.TextBuffer()
			tv = gtk.TextView(buf)
			dlg.vbox.add(tv)
			buf.set_text(option.value)
			dlg.ljedit_get_option = lambda : buf.get_text(buf.get_start_iter(), buf.get_end_iter())
			
		#elif option.data_type=='file':
		#elif option.data_type=='color':
		#elif option.data_type=='list':
		#
		else:
			en = gtk.Entry()
			dlg.vbox.add(en)
			en.set_text(option.value)
			dlg.ljedit_get_option = lambda : en.get_text()
			
		return dlg

	def on_Option_row_actived(self, option_dialog, treeview, path, column):
		option, = treeview.get_model()[path]
		# print option
		
		old = option.value
		
		if option.data_type=='bool':
			option.value = 'true' if option.value=='false' else 'false'
			
		else:
			dlg = self.make_modify_option_dialog(option_dialog, option)
			dlg.show_all()
			res = dlg.run()
			if res==gtk.RESPONSE_OK:
				option.value = dlg.ljedit_get_option()
			elif res==gtk.RESPONSE_REJECT:
				option.value = option.default_value
			dlg.destroy()

		treeview.columns_autosize()
		
		if option.value != old:
			changed_options = option_dialog.changed_options
			cop = changed_options.get(option.id)
			if cop:
				if cop[0]==option.value:
					changed_options.pop(option.id)
				elif cop[1]!=option.value:
					changed_options[option.id] = cop[0], option.value
			else:
				changed_options[option.id] = old, option.value

# options plugin
# 
options_manager = OptionsManager()

def active():
	options_manager.create()

def deactive():
	options_manager.destroy()
