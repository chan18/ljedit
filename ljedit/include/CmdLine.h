// CmdLine.h
// 

#ifndef LJED_INC_CMDLINE_H
#define LJED_INC_CMDLINE_H

#include "gtkenv.h"

class CmdLine {
protected:
	virtual ~CmdLine() {}

public:
	class ICallback {
	public:
		virtual ~ICallback() {}
		virtual bool on_active(void* tag) = 0;
		virtual void on_key_changed() = 0;
		virtual bool on_key_press(GdkEventKey* event) = 0;
	};

public:
	void active(ICallback* cb, int x, int y, void* tag=0)
		{ do_active(cb, x, y, tag); }

	void deactive()
		{ do_deactive(); }

	virtual Gtk::Label& label() = 0;
	virtual Gtk::Entry& entry() = 0;

private:
	virtual void do_active(ICallback* cb, int x, int y, void* tag) = 0;
	virtual void do_deactive() = 0;
};

#endif//LJED_INC_CMDLINE_H

