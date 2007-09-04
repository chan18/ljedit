// CmdLine.h
// 

#ifndef LJED_INC_CMDLINE_H
#define LJED_INC_CMDLINE_H

#include "gtkenv.h"

class CmdLine {
public:
	class ICallback {
	public:
		ICallback(CmdLine& cmd_line) : cmd_line_(cmd_line) {}

		virtual void on_active(void* tag) {}

		virtual void on_key_changed(void* tag) {}

		virtual bool on_key_press(GdkEventKey* event, void* tag) {
			switch( event->keyval ) {
			case GDK_Escape:
			case GDK_Return:
				cmd_line_.deactive();
				return true;
			}

			return false;
		}

	protected:
		CmdLine&	cmd_line_;
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

