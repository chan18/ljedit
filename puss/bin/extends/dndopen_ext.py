#!/usr/bin/env python

# dnd open file support
# 

import gtk
import sys

import puss

def on_drag_data_received(w, drag_context, x, y, selection_data, info, time):
	if sys.platform=='win32':
		#win32 selection_data.get_uris() empty
		s = selection_data.data.split('\r\n')
		for uri in s:
			if uri.startswith('file:///'):
				puss.doc_open(uri[8:])
	else:
		for uri in selection_data.get_uris():
			if uri.startswith('file:///'):
				puss.doc_open(uri[7:])

def on_page_added(notebook, page, page_num):
	view = puss.doc_get_view_from_page_num(page_num)
	view.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	view.connect('drag-data-received', on_drag_data_received)

def active():
	mw = puss.main_window
	mw.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	mw.connect('drag-data-received', on_drag_data_received)

	mw.doc_panel.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	mw.doc_panel.connect('drag-data-received', on_drag_data_received)
	mw.doc_panel.connect('page-added', on_page_added)

def deactive():
	pass

