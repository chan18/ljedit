// CmdLineCallbacks.cpp
// 

#ifndef LJED_INC_CMDLINECALLBACKS_H
#define LJED_INC_CMDLINECALLBACKS_H

#include "CmdLine.h"

class CmdGotoCallback : public CmdLine::ICallback {
public:
	CmdGotoCallback(CmdLine& cmd_line) : CmdLine::ICallback(cmd_line) {}

	virtual void on_active(void* tag);
	virtual void on_key_changed(void* tag);
};

class CmdFindCallback : public CmdLine::ICallback {
public:
	CmdFindCallback(CmdLine& cmd_line) : CmdLine::ICallback(cmd_line) {}

	virtual void on_active(void* tag);
	virtual void on_key_changed(void* tag);
	virtual bool on_key_press(GdkEventKey* event, void* tag);
};

#endif//LJED_INC_CMDLINECALLBACKS_H

