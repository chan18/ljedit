// CmdLineCallbacks.cpp
// 

#ifndef LJED_INC_CMDLINECALLBACKS_H
#define LJED_INC_CMDLINECALLBACKS_H

#include "CmdLine.h"

class DocPageImpl;
class DocManagerImpl;

class BaseCmdCallback : public CmdLine::ICallback {
public:
	BaseCmdCallback(CmdLine& cmd_line, DocManagerImpl& doc_mgr)
		: cmd_line_(cmd_line)
		, doc_mgr_(doc_mgr)
		, current_page_(0) {}

protected:
	bool base_active(void* tag);

	bool base_on_key_press(GdkEventKey* event);

protected:
	CmdLine&		cmd_line_;
	DocManagerImpl& doc_mgr_;
	DocPageImpl*	current_page_;
	Gtk::TextIter	last_pos_;
	void*			tag_;
};

class CmdGotoCallback : public BaseCmdCallback {
public:
	CmdGotoCallback(CmdLine& cmd_line, DocManagerImpl& doc_mgr)
		: BaseCmdCallback(cmd_line, doc_mgr) {}

	virtual bool on_active(void* tag);
	virtual bool on_key_changed();
	virtual bool on_key_press(GdkEventKey* event);
};

class CmdFindCallback : public BaseCmdCallback {
public:
	CmdFindCallback(CmdLine& cmd_line, DocManagerImpl& doc_mgr)
		: BaseCmdCallback(cmd_line, doc_mgr) {}

	virtual bool on_active(void* tag);
	virtual bool on_key_changed();
	virtual bool on_key_press(GdkEventKey* event);
};

#endif//LJED_INC_CMDLINECALLBACKS_H

