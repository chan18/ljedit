-- file_explorer.lua

require('lfs')

local self = {}

self.active = function()
	-- print(glib)
	-- print(gtk)
	
	-- for k in lfs.dir('c:\\') do
	-- 	print('~~~', k)
	-- end

	local builder = gtk.Builder.new()
	local ui_file = puss.get_plugins_path() .. '/explorer.ui'
	print( ui_file )
	builder:add_from_file(ui_file)
	self.panel = builder:get_object('panel')
	self.panel:show_all()
	puss.panel_append(self.panel, gtk.Label.new('Explorer2'), "lua_explorer_plugin_panel", 1)
	print('panel', self.panel)
end

self.deactive = function()
	print('panel', self.panel)
	puss.panel_remove(self.panel)
end

return self

