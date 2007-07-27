# _ljedit_py.py
# 


import _ljedit, ljedit

import gtk, os, sys

class __Scope:
	pass

ljedit.main_window = __Scope()
ljedit.main_window.doc_manager  = __Scope()
ljedit.main_window.left_panel   = __Scope()
ljedit.main_window.right_panel  = __Scope()
ljedit.main_window.bottom_panel = __Scope()
ljedit.main_window.status_bar   = __Scope()

ljedit.main_window.doc_manager.create_new_file    = lambda :              _ljedit.ljedit_doc_manager_create_new_file(_ljedit.c_main_window_doc_manager)
ljedit.main_window.doc_manager.open_file          = lambda file, line=0 : _ljedit.ljedit_doc_manager_open_file(_ljedit.c_main_window_doc_manager, filepath, line)
ljedit.main_window.doc_manager.save_current_file  = lambda :              _ljedit.ljedit_doc_manager_save_current_file(_ljedit.c_main_window_doc_manager)
ljedit.main_window.doc_manager.close_current_file = lambda :              _ljedit.ljedit_doc_manager_close_current_file(_ljedit.c_main_window_doc_manager)
ljedit.main_window.doc_manager.save_all_files     = lambda :              _ljedit.ljedit_doc_manager_save_all_files(_ljedit.c_main_window_doc_manager)
ljedit.main_window.doc_manager.close_all_files    = lambda :              _ljedit.ljedit_doc_manager_close_all_files(_ljedit.c_main_window_doc_manager)

ljedit.main_window.left_panel.append_page         = lambda name, widget : _ljedit.gtkmm_notebook_append_page(_ljedit.c_main_window_left_panel, name, widget)
ljedit.main_window.left_panel.remove_page         = lambda page         : _ljedit.gtkmm_notebook_remove_page(_ljedit.c_main_window_left_panel, page)
ljedit.main_window.right_panel.append_page        = lambda name, widget : _ljedit.gtkmm_notebook_append_page(_ljedit.c_main_window_right_panel, name, widget)
ljedit.main_window.right_panel.remove_page        = lambda page         : _ljedit.gtkmm_notebook_remove_page(_ljedit.c_main_window_right_panel, page)
ljedit.main_window.bottom_panel.append_page       = lambda name, widget : _ljedit.gtkmm_notebook_append_page(_ljedit.c_main_window_bottom_panel, name, widget)
ljedit.main_window.bottom_panel.remove_page       = lambda page         : _ljedit.gtkmm_notebook_remove_page(_ljedit.c_main_window_bottom_panel, page)

ljedit.main_window.status_bar.push                = lambda msg, id=0    : _ljedit.gtkmm_status_bar_push(_ljedit.c_main_window_status_bar, msg, id)

ljedit.trace = lambda msg : ljedit.main_window.status_bar.push(str(msg))

def show_msgbox(message):
	dlg = gtk.MessageDialog(parent=None, buttons=gtk.BUTTONS_YES_NO, message_format=str(message))
	dlg.run()
	dlg.destroy()

ljedit.msgbox = show_msgbox 


ljedit_plugins = []

def load_plugin(plugin_file):
	plugin = __import__(plugin_file)
	plugin.active()
	return plugin

PY_PLUGIN_SIGN = '.py.ljedit_plugin'

def load_plugins():
	global ljedit_plugins

	path = os.path.dirname(__file__) + '/plugins'
	import sys
	sys.path.append(path)

	for f in os.listdir(path):
		if f.endswith('.py.ljedit_plugin'):
			try:
				plugin_file = f[:-len(PY_PLUGIN_SIGN)]
				plugin = load_plugin(plugin_file)
				ljedit_plugins.append(plugin)
			except Exception, e:
				ljedit.trace(e)

def unload_plugins():
	global ljedit_plugins
	for plugin in ljedit_plugins:
		try:
			plugin.deactive()
		except Exception, e:
			pass
	del ljed_plugins

