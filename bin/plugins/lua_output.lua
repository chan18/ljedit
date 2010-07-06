require("lgob.gobject")
require("lgob.gtk")
require("lgob.gtksourceview")

local self = {}

self.TIPS = [[-- Welcome to use lua output plugin!
-- usage :
--    press <<run>> button to run current file as lua script
-- globals :
--    trace(...) - trace messages into this output view
--    dump(obj)  - trace table dump into this output view
--    puss - editor lua interface table, use dump(puss) to see puss interface functions
-- notice :
--    1. debug version only, restart this panel use <tools><plugins> restart lua_output plugin, I'm not lua expert.
--    2. write lua plugin NOT use GObject.connect function directly, use puss.safe_connect!!!
--		puss.safe_connect = function(obj, signal, fn, data, error_return_value)
--			function wrapper(...)
--				r, t = pcall(fn, ...)
--				if r then
--					return t
--				else
--					print('Lua Error:', t)
--					return error_return_value
--				end
--			end
--			return obj:connect(signal, wrapper, data)
--		end
]]

self.trace = function(...)
	local text = ''
	for k,v in ipairs(arg) do
		if text=='' then
			text = tostring(v)
		else
			text = text .. ' ' .. tostring(v)
		end
	end
	self.buffer:insert_at_cursor(text, #text)
	self.buffer:insert_at_cursor('\n', 1)
end

self.dump = function(t)
	local text = ''
	for i,v in pairs(t) do
		-- TODO : different color key
		text = text .. i .. ' : ' .. tostring(v) .. '\n'
	end
	self.buffer:insert_at_cursor(text, #text)
	self.buffer:insert_at_cursor('\n', 1)
end

self.on_run_button_clicked = function()
	local doc_panel = puss.get_ui_builder():get_object('doc_panel')
	local page_num = doc_panel:get_current_page()
	if page_num < 0 then
		self.trace('need lua file opened!')
		return
	end

	local buf = puss.doc_get_buffer_from_page_num(page_num);
	local text = buf:get('text')

	self.buffer:set('text', '')
	fn = loadstring(text)

	self.trace('-- run start')
	r, t = pcall(fn)
	if r then
		self.trace('-- run succeed')
	else
		self.trace(t)
		self.trace('-- run failed')
	end
end

self.on_new_button_clicked = function()
	local page_num = puss.doc_new()
	local lang = gtk.source_language_manager_get_default():get_language('lua')
	local buffer = puss.doc_get_buffer_from_page_num(page_num)
	buffer:set_language(lang)
	buffer:set('text', self.TIPS)
end

self.active = function()
	-- self.test()

	local builder = gtk.Builder.new()
	local ui_file = puss.get_plugins_path() .. '/lua_output.glade'
	builder:add_from_file(ui_file)

	local run_button = builder:get_object('run_button')
	local new_lua_file_button = builder:get_object('new_lua_file_button')
	puss.safe_connect(run_button, 'clicked', self.on_run_button_clicked)
	puss.safe_connect(new_lua_file_button, 'clicked', self.on_new_button_clicked)
	self.panel = builder:get_object('panel')
	self.view  = builder:get_object('view')
	self.buffer  = builder:get_object('buffer')
	self.panel:show_all()
	puss.panel_append(self.panel, gtk.Label.new('LuaOutput'), "lua_output_plugin_panel", 3)

	_G.trace = _G.trace or self.trace
	_G.dump = _G.dump or self.dump

	self.trace(self.TIPS)
end

self.deactive = function()
	puss.panel_remove(self.panel)
	if _G.trace==self.trace then _G.trace = nil end
	if _G.dump==self.dump then _G.dump = nil end
end

return self

