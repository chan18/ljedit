// CmdLineCallbacks.cpp
// 

#ifndef LJED_INC_CMDLINECALLBACKS_H
#define LJED_INC_CMDLINECALLBACKS_H

#include "CmdLine.h"

class CmdGotoCallback : public CmdLine::ICallback {
public:
	CmdGotoCallback(CmdLine& cmd_line) : cmd_line_(cmd_line) {}

	virtual void on_active();
	virtual void on_key_changed();
	virtual bool on_key_press(GdkEventKey* event);

private:
	CmdLine& cmd_line_;
};

class CmdFindCallback : public CmdLine::ICallback {
public:
	CmdFindCallback(CmdLine& cmd_line) : cmd_line_(cmd_line) {}

	virtual void on_active();
	virtual void on_key_changed();
	virtual bool on_key_press(GdkEventKey* event);

private:
	CmdLine& cmd_line_;
};

#endif//LJED_INC_CMDLINECALLBACKS_H

