# _ljedit_py.py
# 


import ljedit

import gtk, os, sys

ljedit.main_window.doc_manager.create_new_file    = lambda : ljedit.ljedit_doc_manager_create_new_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.open_file          = lambda filepath, line : ljedit.ljedit_doc_manager_open_file(ljedit.__c_ljedit, filepath, line)
ljedit.main_window.doc_manager.save_current_file  = lambda : ljedit.ljedit_doc_manager_save_current_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_current_file = lambda : ljedit.ljedit_doc_manager_close_current_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.save_all_files     = lambda : ljedit.ljedit_doc_manager_save_all_files(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_all_files    = lambda : ljedit.ljedit_doc_manager_close_all_files(ljedit.__c_ljedit)

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
				ljedit.msgbox(e)

				ljedit.trace(e)

def unload_plugins():
	global ljedit_plugins
	for plugin in ljedit_plugins:
		try:
			plugin.deactive()
		except Exception, e:
			pass
	del ljed_plugins

