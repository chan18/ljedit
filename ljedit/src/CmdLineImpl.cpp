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
	desc_.set_use_markup();
	//desc_.set_text("no desc");

	entry_.signal_changed().connect( sigc::mem_fun(this, &CmdLineImpl::on_key_changed) );
	entry_.signal_key_press_event().connect( sigc::mem_fun(this, &CmdLineImpl::on_key_press), false );

	signal_button_press_event().connect( sigc::bind_return(sigc::hide(sigc::mem_fun(this, &CmdLineImpl::deactive)), false), false );
	main_window.signal_focus_out_event().connect( sigc::bind_return(sigc::hide(sigc::mem_fun(this, &CmdLineImpl::deactive)), false), false );

	Gtk::Table* table = Gtk::manage(new Gtk::Table(2, 2));
	table->attach(label_, 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);
	table->attach(entry_, 1, 2, 0, 1);
	table->attach(desc_, 1, 2, 1, 2);
	table->set_border_width(3);
	table->set_row_spacing(1, 2);

	Gtk::Frame* frame = Gtk::manage(new Gtk::Frame());
	frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	frame->add(*table);
	frame->show_all();

	desc_.hide();

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
	if( cb_ != 0 )
		cb_->on_key_changed();
}

bool CmdLineImpl::on_key_press(GdkEventKey* event) {
	if( cb_ != 0 )
		return cb_->on_key_press(event);

	return false;
}

