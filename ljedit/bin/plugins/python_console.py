# tee.py
# 

import gtk
import ljedit
from console import PythonConsole

python_console = PythonConsole(namespace = {'__builtins__' : __builtins__, 'ljedit' : ljedit})

def active():
	python_console.eval('print "You can access the main window through" ', False)
	bottom = ljedit.main_window.bottom_panel
	python_console.show_all()
	
	python_console.page_id = bottom.append_page('Python Console', python_console)

def deactive():
	bottom = ljedit.main_window.bottom_panel
	bottom.remove_page(python_console.page_id)


