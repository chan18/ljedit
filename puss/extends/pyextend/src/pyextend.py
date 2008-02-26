# pyextend.py

import puss

import os, sys, traceback

py_extends = []

def load_python_extend(filepath):
	ext = __import__(filepath)
	ext.active()
	return ext

def puss_load_python_extends():
	path = os.path.dirname(__file__)

	for f in os.listdir(path):
		if not f.endswith('_ext.py'):
			continue

		try:
			ext = load_python_extend(f[:-3])
			py_extends.append(ext)

		except Exception:
			print "Exception when load python extend(%s)" % f
			print '-'*60
			traceback.print_exc(file=sys.stdout)
			print '-'*60

def puss_unload_python_extends():
	for ext in py_extends:
		try:
			ext.deactive()
		except Exception:
			print "Exception when load python extend(%s)" % ext.__file__
			print '-'*60
			traceback.print_exc(file=sys.stdout)
			print '-'*60

