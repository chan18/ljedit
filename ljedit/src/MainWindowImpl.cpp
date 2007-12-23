// MainWindowImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "MainWindowImpl.h"

#include "RuntimeException.h"
#include "LJEditorUtilsImpl.h"

#include <sstream>


class ToolMenuButtonAction : public Gtk::Action {
protected:
	ToolMenuButtonAction() {}

	explicit ToolMenuButtonAction(const Glib::ustring& name, const Gtk::StockID& stock_id, const Glib::ustring& label = Glib::ustring(), const Glib::ustring& tooltip = Glib::ustring())
		: Gtk::Action(name, stock_id, label, tooltip) {}

public:
	static Glib::RefPtr<Action> create(const Glib::ustring& name, const Gtk::StockID& stock_id, const Glib::ustring& label =  Glib::ustring(), const Glib::ustring& tooltip =  Glib::ustring()) {
		ToolMenuButtonAction* action = new ToolMenuButtonAction(name, stock_id, label, tooltip);
		return Glib::RefPtr<Action>(action);
	}

protected: 
	virtual Gtk::Widget* create_tool_item_vfunc()
		{ return new Gtk::MenuToolButton(); }
};


MainWindowImpl::MainWindowImpl()
	: doc_manager_(*this)
	, cmd_cb_goto_(cmd_line_, doc_manager_)
	, cmd_cb_find_(cmd_line_, doc_manager_)
{
}

MainWindowImpl::~MainWindowImpl() {
}

void MainWindowImpl::create(const std::string& path) {
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox());
    add(*vbox);

    // create main-ui
    create_ui_manager(/*path + "/conf/menu.xml"*/);

    // menubar
    Gtk::Widget* menubar = ui_manager_->get_widget("/MenuBar");
    if( menubar==0 )
        throw RuntimeException("error : create main window not find MenuBar!");
    vbox->pack_start(*menubar, Gtk::PACK_SHRINK);

    // toolbar
    Gtk::Toolbar* toolbar = dynamic_cast<Gtk::Toolbar*>(ui_manager_->get_widget("/ToolBar"));
    if( toolbar==0 )
        throw RuntimeException("error : create main window not find ToolBar!");
	toolbar->set_toolbar_style(Gtk::TOOLBAR_BOTH_HORIZ);
    vbox->pack_start(*toolbar, Gtk::PACK_SHRINK);

	// cmd line
	cmd_line_.create(*this);

	// body hpanel
	Gtk::HPaned* hpaned = Gtk::manage(new Gtk::HPaned());
	hpaned->set_border_width(3);
    vbox->pack_start(*hpaned);
	{
		// left
		left_panel_.set_tab_pos(Gtk::POS_BOTTOM);
		hpaned->pack1(left_panel_, false, false);

		// 
		Gtk::VPaned* right_vpaned = Gtk::manage(new Gtk::VPaned());
		//right_vpaned->set_position(500);
		hpaned->pack2(*right_vpaned, true, false);
		{
			// 
			Gtk::HPaned* right_hpaned = Gtk::manage(new Gtk::HPaned());
			//right_hpaned->set_position(600);
			right_vpaned->pack1(*right_hpaned, true, false);
			{
				// doc manager
				doc_manager_.set_scrollable();
				right_hpaned->pack1(doc_manager_, true, false);

				doc_manager_.create(path);

				// right
				right_panel_.set_size_request(200, -1);
				right_panel_.set_tab_pos(Gtk::POS_BOTTOM);
				right_hpaned->pack2(right_panel_, false, false);
			}

			// bottom
			bottom_panel_.set_size_request(-1, 200);
			bottom_panel_.set_tab_pos(Gtk::POS_BOTTOM);
			right_vpaned->pack2(bottom_panel_, false, false);
		}

	}

    // statusbar
    vbox->pack_start(status_bar_, false, false);

    resize(1024, 768);
	show_all_children();
}

void MainWindowImpl::create_ui_manager(/*const std::string& config_file*/) {
    action_group_ = Gtk::ActionGroup::create("LJEditActions");

	// main menu
    action_group_->add( Gtk::Action::create("FileMenu", "_File") );
    action_group_->add( Gtk::Action::create("EditMenu", "_Edit") );
    action_group_->add( Gtk::Action::create("ViewMenu", "_View") );
    action_group_->add( Gtk::Action::create("ToolsMenu", "_Tools") );
    action_group_->add( Gtk::Action::create("PluginsMenu", "_Plugins") );
    action_group_->add( Gtk::Action::create("HelpMenu", "_Help") );

	// file menu
    action_group_->add( Gtk::Action::create("New",         Gtk::Stock::NEW,        "_New",        "Create a new file"),                                         sigc::mem_fun(doc_manager_, &DocManager::create_new_file) );
    action_group_->add( Gtk::Action::create("Open",        Gtk::Stock::OPEN,       "_Open",       "Open a file"),                                               sigc::mem_fun(doc_manager_, &DocManager::show_open_dialog) );
    action_group_->add( Gtk::Action::create("Save",        Gtk::Stock::SAVE,       "_Save",       "Save current file"),                                         sigc::mem_fun(doc_manager_, &DocManager::save_current_file) );
    action_group_->add( Gtk::Action::create("SaveAs",      Gtk::Stock::SAVE,       "Save _As...", "Save to a file"),       Gtk::AccelKey("<control><shift>S"),  sigc::mem_fun(doc_manager_, &DocManager::save_current_file_as) );
    action_group_->add( Gtk::Action::create("Close",       Gtk::Stock::CLOSE,      "_Close",      "Close current file"),                                        sigc::mem_fun(doc_manager_, &DocManager::close_current_file) );

    action_group_->add( Gtk::Action::create("Quit",        Gtk::Stock::QUIT,       "_Quit",       "Quit"),                                                      sigc::mem_fun(this, &MainWindowImpl::on_file_quit) );

	// edit menu
	action_group_->add( Gtk::Action::create("CmdLineFind", Gtk::Stock::FIND,       "_Find",       "Active command line control"), Gtk::AccelKey("<control>K"),  sigc::mem_fun(this, &MainWindowImpl::on_edit_find) );
	action_group_->add( Gtk::Action::create("CmdLineGoto", Gtk::Stock::JUMP_TO,    "_Goto",       "Active mini command control"), Gtk::AccelKey("<control>I"),  sigc::mem_fun(this, &MainWindowImpl::on_edit_goto) );
	action_group_->add( Gtk::Action::create("GoBack",      Gtk::Stock::GO_BACK,    "Go_Back",     "Go back position"),		                                    sigc::mem_fun(doc_manager_, &DocManagerImpl::pos_back) );
	action_group_->add( Gtk::Action::create("GoForward",   Gtk::Stock::GO_FORWARD, "Go_Forward",  "Go forward position"),	                                    sigc::mem_fun(doc_manager_, &DocManagerImpl::pos_forward) );

	// view menu
	action_group_->add( Gtk::ToggleAction::create("FullScreen",   Gtk::Stock::INFO, "FullScreen",   "Set/Unset fullscreen",  false), Gtk::AccelKey("F11"),      sigc::mem_fun(this, &MainWindowImpl::on_view_fullscreen) );

	action_group_->add( Gtk::ToggleAction::create("LeftPanel",   Gtk::Stock::INFO, "LeftPanel",   "Show/Hide left panel",    true),                             sigc::mem_fun(this, &MainWindowImpl::on_view_left) );
	action_group_->add( Gtk::ToggleAction::create("RightPanel",  Gtk::Stock::INFO, "RightPanel",  "Show/Hide right panel",   true),	                            sigc::mem_fun(this, &MainWindowImpl::on_view_right) );
	action_group_->add( Gtk::ToggleAction::create("BottomPanel", Gtk::Stock::INFO, "BottomPanel", "Show/Hide bottom panel",  true),	                            sigc::mem_fun(this, &MainWindowImpl::on_view_bottom) );

	//action_group_->add( Gtk::Action::create("NextDocPage", "_Next Document"), Gtk::AccelKey("<control>Tab"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 2) );
	action_group_->add( Gtk::Action::create("BottomPages", Gtk::Stock::INDEX, "Bottom Pages") );
	{
		// view menu / bottom pages
		action_group_->add( Gtk::Action::create("BottomPage1", "bottom page _1"), Gtk::AccelKey("<control>1"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 0) );
		action_group_->add( Gtk::Action::create("BottomPage2", "bottom page _2"), Gtk::AccelKey("<control>2"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 1) );
		action_group_->add( Gtk::Action::create("BottomPage3", "bottom page _3"), Gtk::AccelKey("<control>3"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 2) );
		action_group_->add( Gtk::Action::create("BottomPage4", "bottom page _4"), Gtk::AccelKey("<control>4"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 3) );
		action_group_->add( Gtk::Action::create("BottomPage5", "bottom page _5"), Gtk::AccelKey("<control>5"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 4) );
		action_group_->add( Gtk::Action::create("BottomPage6", "bottom page _6"), Gtk::AccelKey("<control>6"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 5) );
		action_group_->add( Gtk::Action::create("BottomPage7", "bottom page _7"), Gtk::AccelKey("<control>7"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 6) );
		action_group_->add( Gtk::Action::create("BottomPage8", "bottom page _8"), Gtk::AccelKey("<control>8"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 7) );
		action_group_->add( Gtk::Action::create("BottomPage9", "bottom page _9"), Gtk::AccelKey("<control>9"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 8) );
		action_group_->add( Gtk::Action::create("BottomPage0", "bottom page _0"), Gtk::AccelKey("<control>0"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 9) );
	}

	// tools menu

	// tools menu - language select menu
	//
	Glib::ustring languages_ui_string;

	action_group_->add( ToolMenuButtonAction::create("SourceLanguages", Gtk::Stock::INDEX, "Language", "select source language"), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::bottom_panel_active_page), 12) );
	{
		Glib::RefPtr<gtksourceview::SourceLanguageManager> mgr = gtksourceview::SourceLanguageManager::get_default();

		Gtk::RadioAction::Group languages_group;

		std::ostringstream oss;
		Glib::StringArrayHandle ids = mgr->get_language_ids();
		Glib::StringArrayHandle::iterator it = ids.begin();
		Glib::StringArrayHandle::iterator end = ids.end();
		for( ; it!=end; ++it ) {
			oss << "<menuitem action='" << (std::string)*it << "'/>"; // << std::flush;
			
			action_group_->add( Gtk::RadioAction::create(languages_group, *it, *it), sigc::bind(sigc::mem_fun(this, &MainWindowImpl::on_tool_languages), *it) );
		}

		languages_ui_string = oss.str();
	}

	// help menu
    action_group_->add( Gtk::Action::create("About", Gtk::Stock::ABOUT, "About", "About"), sigc::mem_fun(this, &MainWindowImpl::on_help_about) );

	// ---------------------------------------------------
	// create UI

	Glib::ustring ui_string;
	{
		std::ostringstream oss;
		oss <<
			"<ui>"
				"<menubar name='MenuBar'>"
					"<menu action='FileMenu'>"
						"<menuitem action='New'/>"
						"<menuitem action='Open'/>"
						"<menuitem action='Save'/>"
						"<menuitem action='SaveAs'/>"
						"<menuitem action='Close'/>"
						"<separator/>"
						"<menuitem action='Quit'/>"
					"</menu>"

					"<menu action='EditMenu'>"
						/*
						"<menuitem action='Undo'/>"
						"<menuitem action='Redo'/>"
						"<separator/>"
						"<menuitem action='Cut'/>"
						"<menuitem action='Copy'/>"
						"<separator/>"
						*/
						"<menuitem action='CmdLineFind'/>"
						"<menuitem action='CmdLineGoto'/>"
						"<separator/>"
						"<menuitem action='GoBack'/>"
						"<menuitem action='GoForward'/>"
						"<separator/>"
					"</menu>"

					"<menu action='ViewMenu'>"
						"<menuitem action='FullScreen'/>"
						"<separator/>"
						"<menuitem action='LeftPanel'/>"
						"<menuitem action='RightPanel'/>"
						"<menuitem action='BottomPanel'/>"
						/*
						"<separator/>"
						"<menuitem action='NextDocPage'/>"
						*/
						"<separator/>"
						"<menu action='BottomPages'>"
							"<menuitem action='BottomPage1'/>"
							"<menuitem action='BottomPage2'/>"
							"<menuitem action='BottomPage3'/>"
							"<menuitem action='BottomPage4'/>"
							"<menuitem action='BottomPage5'/>"
							"<menuitem action='BottomPage6'/>"
							"<menuitem action='BottomPage7'/>"
							"<menuitem action='BottomPage8'/>"
							"<menuitem action='BottomPage9'/>"
							"<menuitem action='BottomPage0'/>"
						"</menu>"
					"</menu>"

					"<menu action='ToolsMenu'>"
						"<menu action='SourceLanguages'>"

			<<				languages_ui_string
							/*
							"<menuitem action='c'/>"
							"<menuitem action='c++'/>"
							"<menuitem action='python'/>"
							*/
			<<
						"</menu>"
					"</menu>"

					"<menu action='PluginsMenu'>"
					"</menu>"

					"<menu action='HelpMenu'>"
						"<menuitem action='About'/>"
					"</menu>"

				"</menubar>"

				"<toolbar name='ToolBar'>"
					"<toolitem action='Open'/>"
					"<toolitem action='Quit'/>"
					"<separator/>"
					"<toolitem action='GoBack'/>"
					"<toolitem action='GoForward'/>"
					"<separator/>"
					"<toolitem action='SourceLanguages'>"
						"<menu action='SourceLanguages'>"

			<<				languages_ui_string
							/*
							"<menuitem action='c'/>"
							"<menuitem action='c++'/>"
							"<menuitem action='python'/>"
							*/

			<< 
						"</menu>"
					"</toolitem>"
					/*
					"<separator/>"
					"<toolitem action='LeftPanel'/>"
					"<toolitem action='RightPanel'/>"
					"<toolitem action='BottomPanel'/>"
					*/
				"</toolbar>"
			"</ui>";

			ui_string = oss.str();
	}

	ui_manager_ = Gtk::UIManager::create();
	ui_manager_->add_ui_from_string(ui_string);
    ui_manager_->insert_action_group(action_group_);
    add_accel_group( ui_manager_->get_accel_group() );

	// main window
	signal_delete_event().connect(sigc::mem_fun(this, &MainWindowImpl::on_delete_event), false);
}

void MainWindowImpl::destroy() {
	doc_manager_.pages().clear();
}

bool MainWindowImpl::on_delete_event(GdkEventAny* event) {
	return !doc_manager_.close_all_files();
}

void MainWindowImpl::on_file_quit() {
	if( doc_manager_.close_all_files() )
		hide();
}

void MainWindowImpl::on_edit_find() {
	DocPage* page = doc_manager_.get_current_document();
	if( page==0 )
		return;

    int x = 0, y = 0;
	gtksourceview::SourceView* view = &page->view();
	view->get_window(Gtk::TEXT_WINDOW_TEXT)->get_origin(x, y);

	x -= 16;
	y -= 16;

	cmd_line_.active(&cmd_cb_find_, x, y, view);
}

void MainWindowImpl::on_edit_goto() {
	DocPage* page = doc_manager_.get_current_document();
	if( page==0 )
		return;

    int x = 0, y = 0;
	gtksourceview::SourceView* view = &page->view();
	view->get_window(Gtk::TEXT_WINDOW_TEXT)->get_origin(x, y);

	x -= 16;
	y -= 16;

	cmd_line_.active(&cmd_cb_goto_, x, y, view);
}

void MainWindowImpl::on_view_fullscreen() {
	// now easy implement
	static bool is_fullscreen = false;
	is_fullscreen = !is_fullscreen;
	if( is_fullscreen )
		fullscreen();
	else
		unfullscreen();
}

void MainWindowImpl::on_view_left() {
	if( left_panel_.is_visible() )
		left_panel_.hide();
	else
		left_panel_.show();
}

void MainWindowImpl::on_view_right() {
	if( right_panel_.is_visible() )
		right_panel_.hide();
	else
		right_panel_.show();
}

void MainWindowImpl::on_view_bottom() {
	if( bottom_panel_.is_visible() )
		bottom_panel_.hide();
	else
		bottom_panel_.show();
}

void MainWindowImpl::on_tool_languages(Glib::ustring id) {
	DocPageImpl* page = doc_manager_.get_current_doc_page();
	if( page != 0 ) {
		Glib::RefPtr<gtksourceview::SourceLanguageManager> mgr = gtksourceview::SourceLanguageManager::get_default();
		page->source_buffer()->set_language(mgr->get_language(id));
	}
}

void MainWindowImpl::on_help_about() {
	const char* info = "welcome to use ljedit!\n"
		"\n"
		"homepage - <span  foreground='blue'><u>http://ljedit.googlecode.com</u></span>";

	Gtk::MessageDialog(*this, info, true, Gtk::MESSAGE_INFO).run();
}

void MainWindowImpl::bottom_panel_active_page(int page_id) {
	if( page_id > 9 ) {
		page_id += 1;
	}

	bottom_panel_.set_current_page(page_id);

	Gtk::Widget* w = bottom_panel_.get_nth_page(page_id);
	if( w != NULL ) {
		// send focus_in_event, so bottom-panel plugins can use this event
		// 

		GdkEvent* fevent = ::gdk_event_new(GDK_FOCUS_CHANGE);
		if( fevent==0 )
			return;

		fevent->focus_change.type = GDK_FOCUS_CHANGE;
		fevent->focus_change.window = (GdkWindow*)g_object_ref(w->get_window()->gobj());
		fevent->focus_change.in = 1;

		w->event(fevent);

		gdk_event_free(fevent);
	}
}

