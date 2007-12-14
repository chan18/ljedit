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
		action.connect('activate', self.on_Option_activate)
		action_group.add_action(action)
	
		ui_manager = ljedit.main_window.ui_manager
		ui_manager.insert_action_group(action_group, 0)
		self.menu_id = ui_manager.add_ui_from_string(UI_INFO)
		ui_manager.ensure_update()

	def destroy(self):
		ljedit.main_window.ui_manager.remove_ui(self.menu_id)

	def on_Option_activate(self, event):
		ljedit.config_manager
		pass


options_manager = OptionsManager()

def active():
	options_manager.create()

def deactive():
	options_manager.destroy()
