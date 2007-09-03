// CmdLine.h
// 

#ifndef LJED_INC_CMDLINE_H
#define LJED_INC_CMDLINE_H

#include "gtkenv.h"

class CmdLine {
public:
	class ICallback {
	public:
		virtual void on_active() = 0;
		virtual void on_key_changed() = 0;
		virtual bool on_key_press(GdkEventKey* event) = 0;
	};

public:
	virtual Gtk::Label& label() = 0;
	virtual Gtk::Entry& entry() = 0;

	virtual void active(ICallback* cb) = 0;
};

#endif//LJED_INC_CMDLINE_H

