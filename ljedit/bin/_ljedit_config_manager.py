# _ljedit_config_manager.py
# 

import os, sys, pickle

# options manager implements
# 
class Option:
	pass

# option = { id, data_type, value, default_value, info }
# options { id : option }

options_filename = None

options = {}

signal_option_changed = []

def load_options():
	if not os.path.exists(options_filename):
		return

	try:
		f = open(options_filename, 'r')
	except Exception, e:
		print 'warning : open options file failed,', e

	try:
		data = pickle.load(f)
		for id, value in data:
			option = options.get(id)
			if option:
				option.value = value
			else:
				option = Option()
				option.id = id
				option.value = value
				options[id] = option
		
	except Exception, e:
		print 'warning : load options failed,', e

	f.close()

def save_options():
	try:
		f = open(options_filename, 'w')
	except Exception, e:
		print 'warning : open options file failed,', e

	try:
		data = [ (option.id, option.value) for option in options.values() ]
		pickle.dump(data, f)
	except Exception, e:
		print 'warning : save options failed :', e

	f.close()

def regist_ljedit_options():
	regist_option('editor.double_click_select', 'bool', 'true', 'modify default double click events,\nselect word with "_"')
	regist_option('editor.font', 'font', '' , 'set editor font')
	regist_option('editor.tab_width', 'int', '4', 'set editor tab space')

# ljedit ConfigManagerImpl
# 
# ljedit.ConfigManagerImpl add methods:
#     __option_changed_callback(id, value, old)
# 

def create():
	# get options file path
	# 
	if sys.platform=='win32':
		path = os.path.dirname(__file__)
	else:
		home = os.getenv('HOME')
		if home:
			path = home
		else:
			path = os.path.dirname(__file__)
	
	# regist default ljedit options
	# 
	regist_ljedit_options()
	
	# read options file
	# 
	global options_filename
	options_filename = os.path.join(path, '.ljedit.options')

	load_options()

def regist_option(id, data_type, default_value, tip):
	option = options.get(id)
	if option:
		option.data_type = data_type
		option.default_value = default_value
		option.tip = tip
	else:
		option = Option()
		options[id] = option
		option.id = id
		option.data_type = data_type
		option.default_value = default_value
		option.value = default_value
		option.tip = tip

def get_option_value(id):
	option = options.get(id)
	if option:
		return option.value

def option_changed_callback(id, value, old):
	# python callbacks
	for cb in signal_option_changed:
		try:
			cb(id, value, old)
		except Exception, e:
			print 'ERROR : option changed callback failed!'
			print '  FUN :', cb
			print '  ERR :', e
		
	# c++ callbacks
	try:
		__option_changed_callback(id, value, old)
	except Exception, e:
		print 'ERROR : option changed callback failed!'
		print '  FUN :', __option_changed_callback
		print '  ERR :', e
