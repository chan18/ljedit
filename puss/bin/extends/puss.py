# puss.py

import __puss
import gtk

gettext = __puss.__gettext

#builder = __puss.__get_puss_ui_builder(__puss.__app)
#print builder
#print dir(builder)

main_window = __puss.__get_puss_ui_object_by_id(__puss.__app, "main_window")

main_window.ui_manager		= __puss.__get_puss_ui_object_by_id(__puss.__app, "main_ui_manager")
main_window.doc_panel		= __puss.__get_puss_ui_object_by_id(__puss.__app, "doc_panel")
main_window.left_panel		= __puss.__get_puss_ui_object_by_id(__puss.__app, "left_panel")
main_window.right_panel		= __puss.__get_puss_ui_object_by_id(__puss.__app, "right_panel")
main_window.bottom_panel	= __puss.__get_puss_ui_object_by_id(__puss.__app, "bottom_panel")
main_window.statusbar		= __puss.__get_puss_ui_object_by_id(__puss.__app, "statusbar")


def doc_get_url(buf):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_get_url(__puss.__app, buf)

def doc_set_url(buf, url):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_set_url(__puss.__app, buf, url)

def doc_get_charset(buf):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_get_charset(__puss.__app, buf)

def doc_set_charset(buf, charset):
	if not isinstance(buf, gtk.TextBuffer):
		raise TypeError("need gtk.TextBuffer")
	return __puss.__doc_set_charset(__puss.__app, buf, charset)

def doc_get_label_from_page_num(page):		return __puss.__doc_doc_get_label_from_page_num(__puss.__app, page)
def doc_get_view_from_page_num(page):		return __puss.__doc_doc_get_view_from_page_num(__puss.__app, page)
def doc_get_buffer_from_page_num(page):		return __puss.__doc_doc_get_buffer_from_page_num(__puss.__app, page)

def doc_find_page_from_url(url):			return __puss.__doc_find_page_from_url(__puss.__app, url)

def doc_open(url=None, line=-1, offset=-1, show_message_if_open_failed=False):
	return __puss.__doc_open(__puss.__app, url, line, offset, show_message_if_open_failed)

def doc_new():								return __puss.__doc_new(__puss.__app)
def doc_locate(url, line=0, offset=0):		return __puss.__doc_locate(__puss.__app, url, line, offset)
def doc_save_current(is_save_as = False):	return __puss.__doc_save_current(__puss.__app, is_save_as)
def doc_close_current():					return __puss.__doc_close_current(__puss.__app)
def doc_save_all():							return __puss.__doc_save_all(__puss.__app)
def doc_close_all():						return __puss.__doc_close_all(__puss.__app)

def send_focus_change(widget, force_in):
	if not isinstance(widget, gtk.Widget):
		raise TypeError("need gtk.Widget")
	return __puss.__send_focus_change(__puss.__app, widget, force_in)

def active_panel_page(notebook, page):
	if not isinstance(notebook, gtk.Notebook):
		raise TypeError("need gtk.Notebook")
	return __puss.__active_panel_page(__puss.__app, notebook, page)

def option_manager_find(group, key):								return __puss.__option_manager_find(group, key)
def option_manager_option_reg(group, key, default_value, setter):	return __puss.__option_manager_option_reg(group, key, default_value, setter)
def option_manager_monitor_reg(group, key, monitor):				return __puss.__option_manager_monitor_reg(group, key, monitor)

def show_msgbox(message):
	dlg = gtk.MessageDialog(parent=main_window, buttons=gtk.BUTTONS_YES_NO, message_format=repr(message))
	dlg.run()
	dlg.destroy()

