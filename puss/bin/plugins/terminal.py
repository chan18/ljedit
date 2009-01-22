# -*- coding: utf8 -*-

# terminal.py - Embeded VTE terminal for gedit
# This file is part of gedit
#
# Copyright (C) 2005-2006 - Paolo Borelli
#
# gedit is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# gedit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with gedit; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, 
# Boston, MA  02110-1301  USA

import pango
import gtk
import gobject
import vte
import gconf
import os
import gettext

TEXT_DOMAIN = 'puss_py_ext_terminal'

_ = lambda s : gettext.dgettext(TEXT_DOMAIN, s)

gettext.bindtextdomain(TEXT_DOMAIN, os.path.join(os.path.dirname(__file__), '../locale'))
gettext.bind_textdomain_codeset(TEXT_DOMAIN, 'UTF-8')

class Terminal(gtk.HBox):
    GCONF_PROFILE_DIR = "/apps/gnome-terminal/profiles/Default"
    
    defaults = {
        'allow_bold'            : True,
        'audible_bell'          : False,
        'background'            : None,
        'background_color'      : '#000000',
        'backspace_binding'     : 'ascii-del',
        'cursor_blinks'         : False,
        'emulation'             : 'xterm',
        'font_name'             : 'Monospace 10',
        'foreground_color'      : '#AAAAAA',
        'scroll_on_keystroke'   : False,
        'scroll_on_output'      : False,
        'scrollback_lines'      : 100,
        'visible_bell'          : False
    }

    def __init__(self):
        gtk.HBox.__init__(self, False, 4)

        gconf_client.add_dir(self.GCONF_PROFILE_DIR,
                             gconf.CLIENT_PRELOAD_RECURSIVE)

        self._vte = vte.Terminal()
        self.reconfigure_vte()
        self._vte.set_size(self._vte.get_column_count(), 5)
        self._vte.set_size_request(200, 50)
        self._vte.show()
        self.pack_start(self._vte)
        
        self._scrollbar = gtk.VScrollbar(self._vte.get_adjustment())
        self._scrollbar.show()
        self.pack_start(self._scrollbar, False, False, 0)
        
        gconf_client.notify_add(self.GCONF_PROFILE_DIR,
                                self.on_gconf_notification)
        
        self._vte.connect("key-press-event", self.on_vte_key_press)
        self._vte.connect("child-exited", lambda term: term.fork_command())

        self._vte.fork_command()

    def reconfigure_vte(self):
        # Fonts
        if gconf_get_bool(self.GCONF_PROFILE_DIR + "/use_system_font"):
            font_name = gconf_get_str("/desktop/gnome/interface/monospace_font",
                                      self.defaults['font_name'])
        else:
            font_name = gconf_get_str(self.GCONF_PROFILE_DIR + "/font",
                                      self.defaults['font_name'])

        try:
            self._vte.set_font(pango.FontDescription(font_name))
        except:
            pass

        # colors
        fg_color = gconf_get_str(self.GCONF_PROFILE_DIR + "/foreground_color",
                                 self.defaults['foreground_color'])
        bg_color = gconf_get_str(self.GCONF_PROFILE_DIR + "/background_color",
                                 self.defaults['background_color'])
        self._vte.set_colors(gtk.gdk.color_parse (fg_color),
                             gtk.gdk.color_parse (bg_color),
                             [])

        self._vte.set_cursor_blinks(gconf_get_bool(self.GCONF_PROFILE_DIR + "/cursor_blinks",
                                                   self.defaults['cursor_blinks']))

        self._vte.set_audible_bell(not gconf_get_bool(self.GCONF_PROFILE_DIR + "/silent_bell",
                                                      not self.defaults['audible_bell']))

        self._vte.set_scrollback_lines(gconf_get_int(self.GCONF_PROFILE_DIR + "/scrollback_lines",
                                                     self.defaults['scrollback_lines']))
        
        self._vte.set_allow_bold(gconf_get_bool(self.GCONF_PROFILE_DIR + "/allow_bold",
                                                self.defaults['allow_bold']))

        self._vte.set_scroll_on_keystroke(gconf_get_bool(self.GCONF_PROFILE_DIR + "/scroll_on_keystroke",
                                                         self.defaults['scroll_on_keystroke']))

        #self._vte.set_scroll_on_output(gconf_get_bool(self.GCONF_PROFILE_DIR + "/scroll_on_output",
        #                                              self.defaults['scroll_on_output']))

        self._vte.set_scroll_on_output(True)

        self._vte.set_emulation(self.defaults['emulation'])
        self._vte.set_visible_bell(self.defaults['visible_bell'])

    def on_gconf_notification(self, client, cnxn_id, entry, what):
        self.reconfigure_vte()

    def on_vte_key_press(self, term, event):
        modifiers = event.state & gtk.accelerator_get_default_mod_mask()
        if event.keyval in (gtk.keysyms.Tab, gtk.keysyms.KP_Tab, gtk.keysyms.ISO_Left_Tab):
            if modifiers == gtk.gdk.CONTROL_MASK:
                self.get_toplevel().child_focus(gtk.DIR_TAB_FORWARD)
                return True
            elif modifiers == gtk.gdk.CONTROL_MASK | gtk.gdk.SHIFT_MASK:
                self.get_toplevel().child_focus(gtk.DIR_TAB_BACKWARD)
                return True
        return False

gconf_client = gconf.client_get_default()
def gconf_get_bool(key, default = False):
    val = gconf_client.get(key)
    
    if val is not None and val.type == gconf.VALUE_BOOL:
        return val.get_bool()
    else:
        return default

def gconf_get_str(key, default = ""):
    val = gconf_client.get(key)
    
    if val is not None and val.type == gconf.VALUE_STRING:
        return val.get_string()
    else:
        return default

def gconf_get_int(key, default = 0):
    val = gconf_client.get(key)
    
    if val is not None and val.type == gconf.VALUE_INT:
        return val.get_int()
    else:
        return default


if __name__ == "__main__":
	w = gtk.Window()
	w.connect("destroy", gtk.main_quit)
	
	v = Terminal()
	w.add(v)
	w.set_size_request(800, 600)
	w.show_all()
	
	gtk.main()

try:
	import puss

	terminal = Terminal()

	def puss_plugin_active():
		terminal.show_all()

		bottom_panel = puss.ui.get_object("bottom_panel")
		bottom_panel.append_page(terminal, gtk.Label(_('Terminal')))
		terminal.connect('focus_in_event', lambda *args : terminal._vte.grab_focus())

	def puss_plugin_deactive():
		pass

except:
	pass

