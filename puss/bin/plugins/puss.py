# puss.py

import __puss
import gtk

builder = __puss.__builder

main_window = builder.get_object("main_window")

main_window.ui_manager		= builder.get_object("main_ui_manager")
main_window.doc_panel		= builder.get_object("doc_panel")
main_window.left_panel		= builder.get_object("left_panel")
main_window.right_panel		= builder.get_object("right_panel")
main_window.bottom_panel	= builder.get_object("bottom_panel")
main_window.statusbar		= builder.get_object("statusbar")


def doc_get_url(buf):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_get_url(buf)

def doc_set_url(buf, url):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_set_url(buf, url)

def doc_get_charset(buf):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_get_charset(buf)

def doc_set_charset(buf, charset):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_set_charset(buf, charset)

def doc_get_label_from_page_num(page):		return __puss.__doc_doc_get_label_from_page_num(page)
def doc_get_view_from_page_num(page):		return __puss.__doc_doc_get_view_from_page_num(page)
def doc_get_buffer_from_page_num(page):		return __puss.__doc_doc_get_buffer_from_page_num(page)

def doc_find_page_from_url(url):			return __puss.__doc_find_page_from_url(url)

def doc_open(url=None, line=-1, offset=-1, show_message_if_open_failed=False):
	return __puss.__doc_open(url, line, offset, show_message_if_open_failed)

def doc_new():								return __puss.__doc_new(__puss.__app)
def doc_locate(url, line=0, offset=0):		return __puss.__doc_locate(url, line, offset)
def doc_save_current(is_save_as = False):	return __puss.__doc_save_current(is_save_as)
def doc_close_current():					return __puss.__doc_close_current(__puss.__app)
def doc_save_all():							return __puss.__doc_save_all(__puss.__app)
def doc_close_all():						return __puss.__doc_close_all(__puss.__app)

def send_focus_change(widget, force_in):
	if not isinstance(widget, gtk.Widget):
		raise TypeError("need gtk.Widget")
	return __puss.__send_focus_change(widget, force_in)

def active_panel_page(notebook, page):
	if not isinstance(notebook, gtk.Notebook):
		raise TypeError("need gtk.Notebook")
	return __puss.__active_panel_page(notebook, page)

def option_manager_find(group, key):								return __puss.__option_manager_find(group, key)
def option_manager_option_reg(group, key, default_value, setter):	return __puss.__option_manager_option_reg(group, key, default_value, setter)
def option_manager_monitor_reg(group, key, monitor):				return __puss.__option_manager_monitor_reg(group, key, monitor)

def show_msgbox(message):
	dlg = gtk.MessageDialog(parent=main_window, buttons=gtk.BUTTONS_YES_NO, message_format=repr(message))
	dlg.run()
	dlg.destroy()

