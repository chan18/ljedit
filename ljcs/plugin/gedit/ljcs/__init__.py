import sys, os
sys.path.append(os.path.dirname(__file__))

import gedit, gtk, gobject, time, thread, threading
import geditplugin

# plugin
# 

class CpPlugin(gedit.Plugin):
	def __init__(self):
		gedit.Plugin.__init__(self)

	def connect_handlers(self, view, ui):
		ops = geditplugin.LJCSOps(ui)
		doc = view.get_buffer()
		ops.active_plugin(doc, None, view)
		
		ids = []
		doc.set_data('LJCS_PLUGIN_IDS', ids)
		ids.append( (doc, doc.connect('loaded', ops.active_plugin, view)) )
		ids.append( (doc, doc.connect('saved', ops.active_plugin, view)) )
		ids.append( (view, view.connect('focus_out_event', lambda v, e : ops.ui.tip.hide())) )
		ids.append( (view, view.connect('key_press_event', ops.key_press_callback)) )
		ids.append( (view, view.connect('key_release_event', ops.key_release_callback)) )
		ids.append( (view, view.connect('button_release_event', ops.button_release_callback)) )
		ids.append( (view, view.connect('motion_notify_event', ops.motion_notify_callback)) )

	def disconnect_handlers(self, view):
		doc = view.get_buffer()
		ids = doc.get_data('LJCS_PLUGIN_IDS')
		doc.set_data('LJCS_PLUGIN_IDS', None)
		for w, eid in ids:
			w.disconnect(eid)

	def activate(self, window):
		ui = geditplugin.LJCSUI(window)
		tab_added_id = window.connect('tab_added', lambda w, t: self.connect_handlers(t.get_view(), ui))
		tab_removed_id = window.connect('tab_removed', lambda w, t: self.disconnect_handlers(t.get_view()))
		window.set_data('LJCS_PLUGIN_WINDOW_TAGS', (ui, tab_added_id))

		for view in window.get_views(): 
			self.connect_handlers(view, ui)

		# add bottom panel
		bottom = window.get_bottom_panel()
		bottom.add_item(ui.bottom.panel, 'ljcs tip', gtk.STOCK_EXECUTE)
		ui.bottom.panel.show()

		# add project, outline, symbol to side panel
		'''
		side.add_item(ui.project.panel, 'project', gtk.STOCK_EXECUTE)
		side.add_item(ui.outline.panel, 'outline', gtk.STOCK_EXECUTE)
		side.add_item(ui.symbol.panel, 'symbol', gtk.STOCK_EXECUTE)
		ui.project.panel.show()
		ui.outline.panel.show()
		ui.symbol.panel.show()
		'''

	def deactivate(self, window):
		for view in window.get_views():
			self.disconnect_handlers(view)

		ui, tab_added_id = window.get_data('LJCS_PLUGIN_WINDOW_TAGS')
		window.set_data('LJCS_PLUGIN_WINDOW_TAGS', None)
		window.disconnect(tab_added_id)

		# remove from bottom notebook
		bottom = window.get_bottom_panel()
		bottom.remove_item(ui.bottom.panel)
		del ui

