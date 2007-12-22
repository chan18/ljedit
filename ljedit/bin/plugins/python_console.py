# tee.py
# 

import gobject, gtk
import ljedit
from console import PythonConsole

python_console = PythonConsole(namespace = {'__builtins__' : __builtins__, 'ljedit' : ljedit})

def active():
	python_console.eval('print "copy this plugin from gedit!" ', False)
	python_console.show_all()

	bottom = ljedit.main_window.bottom_panel
	python_console.page_id = bottom.append_page(python_console, gtk.Label('Python Console'))
	python_console.__ljedit_active_id = python_console.connect('focus_in_event', lambda *args : python_console.view.grab_focus())

def deactive():
	bottom = ljedit.main_window.bottom_panel
	python_console.disconnect(python_console.__ljedit_active_id)
	bottom.remove_page(python_console.page_id)

