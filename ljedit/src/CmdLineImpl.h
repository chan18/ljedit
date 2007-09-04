// CmdLineImpl.h
// 

#ifndef LJED_INC_CMDLINEIMPL_H
#define LJED_INC_CMDLINEIMPL_H

#include "CmdLine.h"

class CmdLineImpl : public Gtk::Window, public CmdLine {
public:
	CmdLineImpl();
	~CmdLineImpl();

public:
	void create(Gtk::Window& main_window);

	virtual Gtk::Label& label() { return label_; }
	virtual Gtk::Entry& entry() { return entry_; }

	virtual void do_active(ICallback* cb, int x, int y, void* tag);
	virtual void do_deactive();

private:
	void on_key_changed();
	bool on_key_press(GdkEventKey* event);
	void on_editing_done();
	bool on_button_press(GdkEventButton* event);
	bool on_focus_out(GdkEventFocus* event);

private:
	Gtk::Label		label_;
	Gtk::Entry		entry_;

	ICallback*		cb_;
	void*			tag_;
};

#endif//LJED_INC_CMDLINEIMPL_H

