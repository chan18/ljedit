# _ljedit_py.py
# 


import ljedit, __ljedit

import gtk, os, sys

ljedit.main_window.doc_manager.create_new_file    = lambda :                  __ljedit.ljedit_doc_manager_create_new_file(__ljedit.__c_ljedit)
ljedit.main_window.doc_manager.open_file          = lambda filepath, line=0 : __ljedit.ljedit_doc_manager_open_file(__ljedit.__c_ljedit, filepath, line)
ljedit.main_window.doc_manager.save_current_file  = lambda :                  __ljedit.ljedit_doc_manager_save_current_file(__ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_current_file = lambda :                  __ljedit.ljedit_doc_manager_close_current_file(__ljedit.__c_ljedit)
ljedit.main_window.doc_manager.save_all_files     = lambda :                  __ljedit.ljedit_doc_manager_save_all_files(__ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_all_files    = lambda :                  __ljedit.ljedit_doc_manager_close_all_files(__ljedit.__c_ljedit)

ljedit.main_window.doc_manager.get_file_path      = lambda page_num:          __ljedit.ljedit_doc_manager_get_file_path(__ljedit.__c_ljedit, page_num)
ljedit.main_window.doc_manager.get_text_view      = lambda page_num:          __ljedit.ljedit_doc_manager_get_text_view(__ljedit.__c_ljedit, page_num)

ljedit.trace = lambda msg : ljedit.main_window.status_bar.push(str(msg), 0)

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

