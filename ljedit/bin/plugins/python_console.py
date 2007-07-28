# tee.py
# 

import gtk
import ljedit
from console import PythonConsole

python_console = PythonConsole(namespace = {'__builtins__' : __builtins__, 'ljedit' : ljedit})

def active():
	python_console.eval('print "copy this plugin from gedit!" ', False)
	python_console.show_all()

	bottom = ljedit.main_window.bottom_panel
	python_console.page_id = bottom.append_page(python_console, gtk.Label('Python Console'))

def deactive():
	bottom = ljedit.main_window.bottom_panel
	bottom.remove_page(python_console.page_id)
