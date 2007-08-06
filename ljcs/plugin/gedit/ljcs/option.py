# option.py
# 
import os, os.path

from xml.dom.minidom import parse, parseString

HOME_PATH = os.getenv('HOME')
PLUGIN_PATH = os.path.dirname(__file__)
DEFAULT_OPTION_FILE = os.path.join(PLUGIN_PATH, 'ljcs.option')
USER_OPTION_FILE = os.path.join(HOME_PATH, '.ljcs.option')

def parse_option(doc):
	options = {}
	node_options = doc.getElementsByTagName('options')[0]
	node_include_paths = node_options.getElementsByTagName('include_paths')[0]
	list_include_paths = node_include_paths.getElementsByTagName('path')
	
	# parse include paths
	include_paths = []
	for elem in list_include_paths:
		path = elem.getAttribute('value')
		if path.startswith('~/'):	# ~ means user home path
			path = os.path.join(HOME_PATH, path[2:])
		include_paths.append(path)
	#print include_paths
	options['include_paths'] = include_paths
	return options

def parse_text(text):
	doc = parseString(text)
	options = parse_option(doc)
	options['text'] = text
	return options

def load(option_file):
	f = open(option_file, 'r')
	text = f.read()
	f.close()
	
	return parse_text(text)

def load_default_option():
	return load(DEFAULT_OPTION_FILE)

def load_user_option():
	return load(USER_OPTION_FILE)

def save(options):
	if not options.has_key('text'):
		include_paths = options['include_paths']
		text = '<?xml version="1.0"?>\n<options>\n	<include_paths>\n'
		for path in include_paths:
			text += '		<path value="%s"/>\n' % path
		text += '	</include_paths>\n</options>\n'
		options['text'] = text
	
	f = open(USER_OPTION_FILE, 'w+')
	f.write(options['text'])
	f.close()

