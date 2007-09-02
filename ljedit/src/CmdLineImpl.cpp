// CmdLineImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "CmdLineImpl.h"
#include "LJEditorImpl.h"

CmdLineImpl::CmdLineImpl() : cb_(0) {
}

CmdLineImpl::~CmdLineImpl() {
}

void CmdLineImpl::create() {
	label_.set_text("cmd:");
	pack_start(label_, false, false);
	pack_start(entry_);

	entry_.signal_changed().connect( sigc::mem_fun(this, &CmdLineImpl::on_key_changed) );
	entry_.signal_key_press_event().connect( sigc::mem_fun(this, &CmdLineImpl::on_key_press), false );
}

void CmdLineImpl::active(CmdLine::ICallback* cb) {
	cb_ = cb;
	if( cb_ == 0 )
		return;

	cb_->on_active();
}

void CmdLineImpl::on_key_changed() {
	if( cb_ == 0 )
		return;

	cb_->on_key_changed();
}

bool CmdLineImpl::on_key_press(GdkEventKey* event) {
	if( cb_ == 0 )
		return false;

	return cb_->on_key_press(event);
}

