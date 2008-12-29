#!/usr/bin/env python

import os, stat, time, sys
import gobject, gtk

import gettext

TEXT_DOMAIN = 'puss_plugin_miniline'

_ = lambda s : gettext.dgettext(TEXT_DOMAIN, s)

gettext.bindtextdomain(TEXT_DOMAIN, os.path.join(os.path.dirname(__file__), '../locale'))
gettext.bind_textdomain_codeset(TEXT_DOMAIN, 'UTF-8')

import puss

class Miniline:
	pass

miniline = Miniline()

def on_active_miniline(action):
	miniline.panel.show()
	miniline.entry.grab_focus()

def on_changed(entry):
	pass

def on_key_press(entry, event):
	key = event.keyval
	if key in (gtk.keysyms.Return, gtk.keysyms.KP_Enter):
		#puss.show_msgbox(event.keyval)
		miniline.panel.hide()
	elif key==gtk.Escape:
		miniline.panel.hide()

def on_focus_out(entry, event):
	miniline.panel.hide()

def puss_plugin_active():
	miniline.action = gtk.Action("plugin_miniline_action", _("miniline"), _("active miniline."), gtk.STOCK_FIND)
	miniline.action_id = miniline.action.connect('activate', on_active_miniline)

	main_action_group = puss.builder.get_object('main_action_group')
	main_action_group.add_action_with_accel(miniline.action, '<control>K')

	ui_info = """<ui>
		<menubar name='main_menubar'>
		  <menu action='edit_menu'>
		    <placeholder name='edit_menu_plugins_place'>
		      <menuitem action='plugin_miniline_action'/>
		    </placeholder>
		  </menu>
		</menubar>
	  </ui>"""

	ui_mgr = puss.builder.get_object('main_ui_manager')
	miniline.ui_mgr_id = ui_mgr.add_ui_from_string(ui_info)

	ui_info = """<interface>
		<object class="GtkHBox" id="mini_bar_panel">
          <child>
            <object class="GtkImage" id="mini_bar_image">
            </object>
            <packing>
              <property name="expand">false</property>
              <property name="padding">3</property>
              <property name="position">0</property>
            </packing>
          </child>
          <child>
            <object class="GtkEntry" id="mini_bar_entry">
              <signal handler="miniline_cb_changed" name="changed"/>
              <signal handler="miniline_cb_focus_out_event" name="focus-out-event"/>
            </object>
            <packing>
              <property name="expand">false</property>
              <property name="position">1</property>
            </packing>
          </child>
        </object>
	  </interface>"""

	builder = gtk.Builder()
	builder.set_translation_domain(TEXT_DOMAIN)
	builder.add_from_string(ui_info, len(ui_info))
	miniline.panel = builder.get_object('mini_bar_panel')
	miniline.entry = builder.get_object('mini_bar_entry')
	miniline.entry.connect('changed', on_changed)
	miniline.entry.connect('key-press-event', on_key_press)
	miniline.entry.connect('focus-out-event', on_focus_out)
	hbox = puss.builder.get_object('main_toolbar_hbox')
	miniline.panel.show_all()
	miniline.panel.hide()
	hbox.pack_end(miniline.panel, False, False)

	ui_mgr.ensure_update()


def puss_plugin_deactive():
	ui_mgr = puss.builder.get_object('main_ui_manager')
	ui_mgr.remove_ui(miniline.ui_mgr_id)
	main_action_group = puss.builder.get_object('main_action_group')
	main_action_group.remove_action(miniline.action)
	miniline.action.disconnect(miniline.action_id)
