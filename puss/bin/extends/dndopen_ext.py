#!/usr/bin/env python

# dnd open file support
# 

import gtk
import puss

def on_drag_data_received(w, drag_context, x, y, selection_data, info, time):
	for uri in selection_data.get_uris():
		#print uri
		filename = uri.replace('file:///', '/')
		puss.doc_open(filename)
		#print filename

def on_page_added(notebook, page, page_num):
	view = puss.doc_get_view_from_page_num(page_num)
	view.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	page.set_data('DND_OPEN_SIGNAL_ID', (view, view.connect('drag-data-received', on_drag_data_received)))

def on_page_removed(notebook, page, page_num):
	view, id = page.get_data('DND_OPEN_SIGNAL_ID')
	view.disconnect(id)

def active():
	mw = puss.main_window
	mw.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	mw.connect('drag-data-received', on_drag_data_received)

	mw.doc_panel.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	mw.doc_panel.connect('drag-data-received', on_drag_data_received)
	mw.doc_panel.connect('page-added', on_page_added)
	mw.doc_panel.connect('page-removed', on_page_removed)

def deactive():
	pass

