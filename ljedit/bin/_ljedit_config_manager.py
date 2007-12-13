# _ljedit_config_manager.py
# 

import os, sys, pickle

# options manager implements
# 
class Option:
	pass

# option = { data_type, default_value, value }
# options { id : option }

options_filename = None

options = {}

def load_options():
	if os.path.exists(options_filename):
		try:
			options = pickle.load(options_filename)
		except Exception, e:
			print 'load options failed :', e

def save_options():
	try:
		pickle.dump(options, options_filename)
	except Exception, e:
		print 'save options failed :', e

# ljedit ConfigManagerImpl
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
	
	# read options file
	# 
	global options_filename
	options_filename = os.path.join(path, '.ljedit.options')

	load_options()

def regist_option(id, data_type, default_value):
	option = options.get(id)
	if option:
		option.data_type = data_type
		option.default_value = default_value
	else:
		option = Option()
		options[id] = option
		option.id = id
		option.data_type = data_type
		option.default_value = default_value
		option.value = default_value

def get_option_value(id):
	option = options.get(id)
	if option:
		return option.value
