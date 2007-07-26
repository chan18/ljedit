# _ljedit_python_plugins_manager.py
# 


import ljedit

import gtk

ljedit.main_window.doc_manager.create_new_file    = lambda : ljedit.ljedit_doc_manager_create_new_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.open_file          = lambda filepath, line : ljedit.ljedit_doc_manager_open_file(ljedit.__c_ljedit, filepath, line)
ljedit.main_window.doc_manager.save_current_file  = lambda : ljedit.ljedit_doc_manager_save_current_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_current_file = lambda : ljedit.ljedit_doc_manager_close_current_file(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.save_all_files     = lambda : ljedit.ljedit_doc_manager_save_all_files(ljedit.__c_ljedit)
ljedit.main_window.doc_manager.close_all_files    = lambda : ljedit.ljedit_doc_manager_close_all_files(ljedit.__c_ljedit)

def show_debug_window():
	w = gtk.Window()
	v = gtk.TextView()
	w.add(v)
	b = v.get_buffer()
	w.show_all()
	
def load_plugins():
	try:
		show_debug_window()
		dm = ljedit.main_window.doc_manager
		dm.create_new_file()
		import gobject
		gobject.idle_add(dm.open_file, 'D:\\louis\\ljedit\\bin\\_ljedit_python_plugins_manager.py', 18)
	except:
		pass
	raise Exception('v')

def unload_plugins():
	pass

