# _ljedit_python_plugins_manager.py
# 


import ljedit

import gtk, os

class T:
	pass

ljedit.main_window = T()
ljedit.main_window.doc_manager  = T()
ljedit.main_window.left_panel   = T()
ljedit.main_window.right_panel  = T()
ljedit.main_window.bottom_panel = T()

ljedit.main_window.doc_manager.create_new_file    = lambda :                ljedit.ljedit_doc_manager_create_new_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.open_file          = lambda filepath, line : ljedit.ljedit_doc_manager_open_file(ljedit.__c_ljedit, filepath, line)
ljedit.main_window.doc_manager.save_current_file  = lambda :                ljedit.ljedit_doc_manager_save_current_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_current_file = lambda :                ljedit.ljedit_doc_manager_close_current_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.save_all_files     = lambda :                ljedit.ljedit_doc_manager_save_all_files(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_all_files    = lambda :                ljedit.ljedit_doc_manager_close_all_files(ljedit.__c_ljedit)

ljedit.main_window.left_panel.append_page         = lambda name, widget : ljedit.gtkmm_notebook_append_page(ljedit.__c_main_window_left_panel, name, widget)
ljedit.main_window.left_panel.remove_page         = lambda page         : ljedit.gtkmm_notebook_remove_page(ljedit.__c_main_window_left_panel, page)
ljedit.main_window.right_panel.append_page        = lambda name, widget : ljedit.gtkmm_notebook_append_page(ljedit.__c_main_window_right_panel, name, widget)
ljedit.main_window.right_panel.remove_page        = lambda page         : ljedit.gtkmm_notebook_remove_page(ljedit.__c_main_window_right_panel, page)
ljedit.main_window.bottom_panel.append_page       = lambda name, widget : ljedit.gtkmm_notebook_append_page(ljedit.__c_main_window_bottom_panel, name, widget)
ljedit.main_window.bottom_panel.remove_page       = lambda page         : ljedit.gtkmm_notebook_remove_page(ljedit.__c_main_window_bottom_panel, page)

ljedit_plugins = []

def load_plugin(plugin_file):
	plugin = __import__(plugin_file)
	plugin.active()

PY_PLUGIN_SIGN = '.py.ljedit_plugin'

def load_plugins():
	global ljedit_plugins

	path = os.path.dirname(__file__) + '/plugins'
	import sys
	sys.path.append(path)

	for f in os.listdir(path):
		if f.endswith('.py.ljedit_plugin'):
			print 'f:', f
			try:
				plugin_file = f[:-len(PY_PLUGIN_SIGN)]
				print plugin_file
				plugin = load_plugin(plugin_file)
				ljedit_plugins.append(plugin)
			except Exception, e:
				print e
				raise e

def unload_plugins():
	global ljedit_plugins

	for plugin in ljedit_plugins:
		plugin.deactive()
	del ljed_plugins

