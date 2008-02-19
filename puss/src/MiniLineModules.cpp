// MiniLineModules.cpp
// 

#include "MiniLineModules.h"

#include "MiniLine.h"

#include <glib/gi18n.h>
#include <memory.h>

#include "Puss.h"
#include "Utils.h"
#include "DocManager.h"


struct MiniLineGotoImpl {
	

	MiniLineCallback cb;
};

gboolean mini_line_goto_cb_active(Puss* app, gpointer tag) {
	MiniLineGotoImpl* self = (MiniLineGotoImpl*)tag;
	return TRUE;
}

void mini_line_goto_cb_changed(Puss* app, gpointer tag) {
	MiniLineGotoImpl* self = (MiniLineGotoImpl*)tag;
}

gboolean mini_line_goto_cb_key_press(Puss* app, GdkEventKey* event, gpointer tag) {
	MiniLineGotoImpl* self = (MiniLineGotoImpl*)tag;
	return TRUE;
}

MiniLineCallback* puss_mini_line_goto_get_callback() {
	static MiniLineGotoImpl me;
	me.cb.tag = &me;
	me.cb.cb_active = &mini_line_goto_cb_active;
	me.cb.cb_changed = &mini_line_goto_cb_changed;
	me.cb.cb_key_press = &mini_line_goto_cb_key_press;

	return &me.cb;
}
