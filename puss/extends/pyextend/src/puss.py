# puss.py

import __puss
import gtk

main_window = __puss.main_window

main_window.ui_manager		= __puss.ui_manager
main_window.doc_panel		= __puss.doc_panel
main_window.left_panel		= __puss.left_panel
main_window.right_panel		= __puss.right_panel
main_window.bottom_panel	= __puss.bottom_panel
main_window.status_bar		= __puss.status_bar

def doc_get_url(buf):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("not gtk.TextBuffer")
	return __puss.__doc_get_url(__puss.__app, buf)

def doc_get_buffer_from_page_num(page):		return __puss.__doc_doc_get_buffer_from_page_num(__puss.__app, page)

def doc_find_page_from_url(url):			return __puss.__doc_find_page_from_url(__puss.__app, url)

def doc_new():								return __puss.__doc_new(__puss.__app)
def doc_open(url=None, line=0, offset=0):	return __puss.__doc_open(__puss.__app, url, line, offset)
def doc_locate(url, line=0, offset=0):		return __puss.__doc_locate(__puss.__app, url, line, offset)
def doc_save_current(is_save_as = False):	return __puss.__doc_save_current(__puss.__app, is_save_as)
def doc_close_current():					return __puss.__doc_close_current(__puss.__app)
def doc_save_all():							return __puss.__doc_save_all(__puss.__app)
def doc_close_all():						return __puss.__doc_close_all(__puss.__app)

def show_msgbox(message):
	dlg = gtk.MessageDialog(parent=main_window, buttons=gtk.BUTTONS_YES_NO, message_format=str(message))
	dlg.run()
	dlg.destroy()

