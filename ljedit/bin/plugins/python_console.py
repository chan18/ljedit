# tee.py
# 

import gobject, gtk
import ljedit
from console import PythonConsole

python_console = PythonConsole(namespace = {'__builtins__' : __builtins__, 'ljedit' : ljedit})

def on_switch_page(notebook, page, page_num):
	if page_num==python_console.page_id:
		gobject.idle_add(python_console.view.grab_focus)

def active():
	python_console.eval('print "copy this plugin from gedit!" ', False)
	python_console.show_all()

	bottom = ljedit.main_window.bottom_panel
	python_console.page_id = bottom.append_page(python_console, gtk.Label('Python Console'))
	python_console.ljedit_switch_id = ljedit.main_window.bottom_panel.connect('switch_page', on_switch_page)

def deactive():
	bottom = ljedit.main_window.bottom_panel
	bottom.disconnect(python_console.ljedit_switch_id)
	bottom.remove_page(python_console.page_id)


