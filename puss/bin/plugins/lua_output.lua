require("lgob.gobject")
require("lgob.gtk")
require("lgob.gtksourceview")

local UI = [[
<interface>
  <object class="GtkHBox" id="panel">
    <property name="visible">True</property>
    <child>
      <object class="GtkVBox" id="vbox1">
        <property name="visible">True</property>
        <property name="orientation">vertical</property>
        <child>
          <object class="GtkButton" id="run_button">
            <property name="visible">True</property>
            <property name="can_focus">True</property>
            <property name="has_focus">True</property>
            <property name="is_focus">True</property>
            <property name="can_default">True</property>
            <property name="has_default">True</property>
            <property name="receives_default">True</property>
            <child>
              <object class="GtkVBox" id="vbox2">
                <property name="visible">True</property>
                <property name="orientation">vertical</property>
                <child>
                  <object class="GtkImage" id="image1">
                    <property name="visible">True</property>
                    <property name="stock">gtk-media-play</property>
                    <property name="icon-size">6</property>
                  </object>
                  <packing>
                    <property name="position">0</property>
                  </packing>
                </child>
                <child>
                  <object class="GtkLabel" id="label1">
                    <property name="visible">True</property>
                    <property name="label" translatable="yes">run</property>
                  </object>
                  <packing>
                    <property name="position">1</property>
                  </packing>
                </child>
              </object>
            </child>
          </object>
          <packing>
            <property name="expand">False</property>
            <property name="fill">False</property>
            <property name="position">0</property>
          </packing>
        </child>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="fill">False</property>
        <property name="padding">3</property>
        <property name="position">0</property>
      </packing>
    </child>
    <child>
      <object class="GtkScrolledWindow" id="scrolledwindow1">
        <property name="visible">True</property>
        <property name="can_focus">True</property>
        <property name="hscrollbar_policy">automatic</property>
        <property name="vscrollbar_policy">automatic</property>
        <property name="shadow_type">in</property>
        <child>
          <object class="GtkTextView" id="view">
            <property name="visible">True</property>
            <property name="editable">False</property>
            <property name="cursor_visible">False</property>
            <property name="buffer">buffer</property>
          </object>
        </child>
      </object>
      <packing>
        <property name="padding">2</property>
        <property name="position">1</property>
      </packing>
    </child>
  </object>
  <object class="GtkTextBuffer" id="buffer"/>
</interface>
]]

local self = {}

function trace(...)
	local text = ''
	for k,v in ipairs({...}) do
		if text=='' then
			text = tostring(v)
		else
			text = text .. ' ' .. tostring(v)
		end
	end
	self.buffer:insert_at_cursor(text, #text)
	self.buffer:insert_at_cursor('\n', 1)
end

function dump(t)
	local text = ''
	for i,v in pairs(t) do
		-- TODO : different color key
		text = text .. i .. ' : ' .. tostring(v) .. '\n'
	end
	self.buffer:insert_at_cursor(text, #text)
	self.buffer:insert_at_cursor('\n', 1)
end

function on_run_button_clicked()
	local doc_panel = puss.get_ui_builder():get_object('doc_panel')
	local page_num = doc_panel:get_current_page()
	if page_num < 0 then
		trace('need lua file opened!')
		return
	end

	local buf = puss.doc_get_buffer_from_page_num(page_num);
	local text = buf:get('text')

	self.buffer:set('text', '')
	fn = loadstring(text, 'lua')

	trace('-- run start')
	r, t = pcall(fn)
	if r then
		trace('-- run succeed')
	else
		trace(t)
		trace('-- run failed')
	end
end

self.active = function()
	-- self.test()

	local builder = gtk.Builder.new()
	builder:add_from_string(UI, #UI)
	local run_button = builder:get_object('run_button')
	puss.safe_connect(run_button, 'clicked', on_run_button_clicked)
	self.panel = builder:get_object('panel')
	self.view  = builder:get_object('view')
	self.buffer  = builder:get_object('buffer')
	self.panel:show_all()
	puss.panel_append(self.panel, gtk.Label.new('LuaOutput'), "lua_output_plugin_panel", 3)

	trace([[-- Welcome to use lua output plugin!
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
	]])
end

self.deactive = function()
	puss.panel_remove(self.panel)
end

return self

