// CmdLineImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "CmdLineImpl.h"
#include "LJEditorImpl.h"

CmdLineImpl::CmdLineImpl()
	: Gtk::Window(Gtk::WINDOW_POPUP)
	, cb_(0)
{
}

CmdLineImpl::~CmdLineImpl() {
}

void CmdLineImpl::create(Gtk::Window& main_window) {
	label_.set_text("cmd:");

	entry_.signal_changed().connect( sigc::mem_fun(this, &CmdLineImpl::on_key_changed) );
	entry_.signal_key_press_event().connect( sigc::mem_fun(this, &CmdLineImpl::on_key_press), false );

	signal_button_press_event().connect( sigc::mem_fun(this, &CmdLineImpl::on_button_press), false );
	main_window.signal_focus_out_event().connect( sigc::mem_fun(this, &CmdLineImpl::on_focus_out), false );

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
	hbox->pack_start(label_, false, false);
	hbox->pack_start(entry_);
	hbox->set_border_width(3);

	Gtk::Frame* frame = Gtk::manage(new Gtk::Frame());
	frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	frame->add(*hbox);
	frame->show_all();

	add(*frame);
    resize(200, 24);
	set_modal();
}

void CmdLineImpl::do_active(CmdLine::ICallback* cb, int x, int y, void* tag) {
	cb_ = cb;
	if( cb_ == 0 )
		return;

	if( !cb_->on_active(tag) ) {
		deactive();
		return;
	}

	move(x, y);
	show();

	entry_.set_flags( Gtk::HAS_FOCUS );	// show cursor

	GdkEvent* fevent = ::gdk_event_new(GDK_FOCUS_CHANGE);
	if( fevent==0 ) {
		deactive();
		return;
	}

	fevent->focus_change.type = GDK_FOCUS_CHANGE;
	fevent->focus_change.window = (GdkWindow*)g_object_ref(entry_.get_window()->gobj());
	fevent->focus_change.in = 1;

	entry_.event(fevent);

	gdk_event_free(fevent);
}

void CmdLineImpl::do_deactive() {
	hide();
}

void CmdLineImpl::on_key_changed() {
	if( cb_ == 0 )
		return;

	cb_->on_key_changed();
}

bool CmdLineImpl::on_key_press(GdkEventKey* event) {
	if( cb_ != 0 )
		return cb_->on_key_press(event);

	return false;
}

bool CmdLineImpl::on_button_press(GdkEventButton* event) {
	deactive();
	return false;
}

bool CmdLineImpl::on_focus_out(GdkEventFocus* event) {
	deactive();
	return false;
}

