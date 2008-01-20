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

	try:
		os.environ['PATH'] = gtkbin + ';' + os.environ['PATH']
	except KeyError:
		os.environ['PATH'] = gtkbin

	pipe_run('gdk-pixbuf-query-loaders.exe', os.path.join(gtkpath, 'etc\\gtk-2.0\\gdk-pixbuf.loaders'))
	pipe_run('gtk-query-immodules-2.0.exe',  os.path.join(gtkpath, 'etc\\gtk-2.0\\gtk.immodules'))
	pipe_run('pango-querymodules.exe',       os.path.join(gtkpath, 'etc\\pango\\pango.modules'))

	os.execl(os.path.join(path, '_ljedit.exe'))

if __name__=='__main__':
	run()
	print 'vvv'

