// MainWindowImpl.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "MainWindowImpl.h"

#include "RuntimeException.h"


MainWindowImpl::MainWindowImpl()
	: cmd_cb_goto_(cmd_line_, doc_manager_)
	, cmd_cb_find_(cmd_line_, doc_manager_)
{
}

MainWindowImpl::~MainWindowImpl() {
}

void MainWindowImpl::create(const std::string& path) {
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox());
    add(*vbox);

    // create main-ui
    create_ui_manager(path + "/conf/menu.xml");

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

void MainWindowImpl::create_ui_manager(const std::string& config_file) {
    action_group_ = Gtk::ActionGroup::create("Actions");

	// main menu
    action_group_->add( Gtk::Action::create("FileMenu", "_File") );
    action_group_->add( Gtk::Action::create("EditMenu", "_Edit") );
    action_group_->add( Gtk::Action::create("ViewMenu", "_View") );
    action_group_->add( Gtk::Action::create("PluginsMenu", "_Plugins") );
    action_group_->add( Gtk::Action::create("HelpMenu", "_Help") );

	// file menu
    action_group_->add( Gtk::Action::create("New",         Gtk::Stock::NEW,        "_New",        "Create a new file"),                                         sigc::mem_fun(doc_manager_, &DocManager::create_new_file) );
    action_group_->add( Gtk::Action::create("Open",        Gtk::Stock::OPEN,       "_Open",       "Open a file"),                                               sigc::mem_fun(this, &MainWindowImpl::on_file_open) );
    action_group_->add( Gtk::Action::create("Save",        Gtk::Stock::SAVE,       "_Save",       "Save current file"),                                         sigc::mem_fun(doc_manager_, &DocManager::save_current_file) );
    action_group_->add( Gtk::Action::create("SaveAs",      Gtk::Stock::SAVE,       "Save _As...", "Save to a file"),                                            sigc::mem_fun(this, &MainWindowImpl::on_file_save_as) );
    action_group_->add( Gtk::Action::create("Close",       Gtk::Stock::CLOSE,      "_Close",      "Close current file"),                                        sigc::mem_fun(doc_manager_, &DocManager::close_current_file) );
    action_group_->add( Gtk::Action::create("Quit",        Gtk::Stock::QUIT,       "_Quit",       "Quit"),                                                      sigc::mem_fun(this, &MainWindowImpl::on_file_quit) );

	// edit menu
	action_group_->add( Gtk::Action::create("CmdLineFind", Gtk::Stock::FIND,       "_Find",       "Active command line control"), Gtk::AccelKey("<control>K"),  sigc::mem_fun(this, &MainWindowImpl::on_edit_find) );
	action_group_->add( Gtk::Action::create("CmdLineGoto", Gtk::Stock::JUMP_TO,    "_Goto",       "Active mini command control"), Gtk::AccelKey("<control>I"),  sigc::mem_fun(this, &MainWindowImpl::on_edit_goto) );
	action_group_->add( Gtk::Action::create("GoBack",      Gtk::Stock::GO_BACK,    "Go_Back",     "Go back position"),		                                    sigc::mem_fun(doc_manager_, &DocManagerImpl::pos_back) );
	action_group_->add( Gtk::Action::create("GoForward",   Gtk::Stock::GO_FORWARD, "Go_Forward",  "Go forward position"),	                                    sigc::mem_fun(doc_manager_, &DocManagerImpl::pos_forward) );

	// view menu
	action_group_->add( Gtk::ToggleAction::create("LeftPanel",   Gtk::Stock::INFO, "LeftPanel",   "Show/Hide left panel",    true),                             sigc::mem_fun(this, &MainWindowImpl::on_view_left) );
	action_group_->add( Gtk::ToggleAction::create("RightPanel",  Gtk::Stock::INFO, "RightPanel",  "Show/Hide right panel",   true),	                            sigc::mem_fun(this, &MainWindowImpl::on_view_right) );
	action_group_->add( Gtk::ToggleAction::create("BottomPanel", Gtk::Stock::INFO, "BottomPanel", "Show/Hide bottom panel",  true),	                            sigc::mem_fun(this, &MainWindowImpl::on_view_bottom) );

	// help menu
    action_group_->add( Gtk::Action::create("About",       Gtk::Stock::ABOUT,      "About",       "About"),				                                        sigc::mem_fun(this, &MainWindowImpl::on_help_about) );

    ui_manager_ = Gtk::UIManager::create();
    ui_manager_->insert_action_group(action_group_);
    add_accel_group( ui_manager_->get_accel_group() );

    ui_manager_->add_ui_from_file(config_file);
}

void MainWindowImpl::destroy() {
	doc_manager_.close_all_files();
}

void MainWindowImpl::on_file_open() {
    Gtk::FileChooserDialog dlg("open...");
    dlg.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
    dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dlg.set_select_multiple();
    if( dlg.run()!=Gtk::RESPONSE_OK )
        return;

    Glib::SListHandle<Glib::ustring> filenames = dlg.get_filenames();
    Glib::SListHandle<Glib::ustring>::iterator it = filenames.begin();
    Glib::SListHandle<Glib::ustring>::iterator end = filenames.end();
    for( ; it!=end; ++it )
        doc_manager_.open_file(*it);
}

void MainWindowImpl::on_file_save_as() {
    Gtk::MessageDialog(*this, __FUNCTION__).run();
}

void MainWindowImpl::on_file_quit() {
	destroy();

	Gtk::Main::quit();
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



void MainWindowImpl::on_help_about() {
    Gtk::MessageDialog(*this, __FUNCTION__).run();
}

