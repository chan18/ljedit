# boot.py
# 

import os, sys
from subprocess import *

def pipe_run(i, o):
	p = Popen([i], stdout=PIPE, shell=True)	#os.popen(i)
	t = p.communicate('\n')[0]	#t = p.read()
	#p.close()
	f = open(o, 'wb')
	f.write(t)
	f.close()

def run():
	path = os.path.dirname(__file__)
	#print __file__, path
	gtkpath = os.path.join(path, 'environ\\gtk')
	pypath  = os.path.join(path, 'environ\\pylibs')
	gtkbin  = os.path.join(gtkpath, 'bin')

	os.environ['GTK_BASEPATH'] = gtkpath
	os.environ['GTKMM_BASEPATH'] = gtkpath
	os.environ['PYTHONPATH'] = pypath
	os.environ['PATH'] = gtkbin

	i = os.path.join(gtkbin, 'gdk-pixbuf-query-loaders.exe')
	o = os.path.join(gtkpath, 'etc\\gtk-2.0\\gdk-pixbuf.loaders')
	pipe_run(i, o)

	i = os.path.join(gtkbin, 'gtk-query-immodules-2.0.exe')
	o = os.path.join(gtkpath, 'etc\\gtk-2.0\\gtk.immodules')
	pipe_run(i, o)

	i = os.path.join(gtkbin, 'pango-querymodules.exe')
	o = os.path.join(gtkpath, 'etc\\pango\\pango.modules')
	pipe_run(i, o)

	ljedit = os.path.join(path, '_ljedit.exe')
	os.execl(ljedit)

if __name__=='__main__':
	run()
	print 'vvv'

