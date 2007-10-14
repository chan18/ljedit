#!/usr/bin/env python

# dnd open file support
# 

import gtk
import ljedit

sigs = []

def on_drag_data_received(w, drag_context, x, y, selection_data, info, time):
	for uri in selection_data.get_uris():
		#print uri
		filename = uri.replace('file:///', '/')
		ljedit.main_window.doc_manager.open_file(filename)
		#print filename

def on_page_added(notebook, page, page_num):
	view = ljedit.main_window.doc_manager.get_text_view(page_num)
	view.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	page.set_data('DND_OPEN_SIGNAL_ID', (view, view.connect('drag-data-received', on_drag_data_received)))
	

def on_page_removed(notebook, page, page_num):
	view, id = page.get_data('DND_OPEN_SIGNAL_ID')
	view.disconnect(id)

def active():
	mw = ljedit.main_window
	mw.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	sigs.append( (mw, mw.connect('drag-data-received', on_drag_data_received)) )

	dm = mw.doc_manager
	dm.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	sigs.append( (dm, dm.connect('drag-data-received', on_drag_data_received)) )

	sigs.append( (dm, dm.connect('page-added', on_page_added)) )
	sigs.append( (dm, dm.connect('page-removed', on_page_removed)) )

def deactive():
	for w, s in sigs:
		w.disconnect(s)

