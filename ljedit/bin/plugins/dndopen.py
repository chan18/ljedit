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

def active():
	mw = ljedit.main_window
	mw.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	sigs.append( (mw, mw.connect('drag-data-received', on_drag_data_received)) )

	dm = mw.doc_manager
	dm.drag_dest_set(gtk.DEST_DEFAULT_ALL, [('text/uri-list', 0, 0),], gtk.gdk.ACTION_COPY)
	sigs.append( (dm, dm.connect('drag-data-received', on_drag_data_received)) )

def deactive():
	for w, s in sigs:
		w.disconnect(s)

