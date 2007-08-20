// MainWindowImpl.cpp
// 

#include "MainWindowImpl.h"

#include "RuntimeException.h"


MainWindowImpl::MainWindowImpl()
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
    Gtk::Widget* toolbar = ui_manager_->get_widget("/ToolBar");
    if( toolbar==0 )
        throw RuntimeException("error : create main window not find ToolBar!");
    vbox->pack_start(*toolbar, Gtk::PACK_SHRINK);

	// body hpanel
	Gtk::HPaned* hpaned = Gtk::manage(new Gtk::HPaned());
	hpaned->set_border_width(3);
    vbox->pack_start(*hpaned);
	{
		// left
		left_panel_.set_tab_pos(Gtk::POS_BOTTOM);
		hpaned->add1(left_panel_);

		// 
		Gtk::VPaned* right_vpaned = Gtk::manage(new Gtk::VPaned());
		right_vpaned->set_position(500);
		hpaned->add2(*right_vpaned);
		{
			// 
			Gtk::HPaned* right_hpaned = Gtk::manage(new Gtk::HPaned());
			right_hpaned->set_position(600);
			right_vpaned->add1(*right_hpaned);
			{
				// doc manager
				doc_manager_.set_scrollable();
				right_hpaned->add1(doc_manager_);

				// right
				right_panel_.set_size_request(200, -1);
				right_panel_.set_tab_pos(Gtk::POS_BOTTOM);
				right_hpaned->add2(right_panel_);
			}

			// bottom
			bottom_panel_.set_size_request(-1, 200);
			bottom_panel_.set_tab_pos(Gtk::POS_BOTTOM);
			right_vpaned->add2(bottom_panel_);
		}

	}

    // statusbar
    vbox->pack_start(status_bar_, false, false);

    resize(1024, 768);
    show_all();
}

void MainWindowImpl::create_ui_manager(const std::string& config_file) {
    action_group_ = Gtk::ActionGroup::create("Actions");

    action_group_->add( Gtk::Action::create("FileMenu", "_File") );
    action_group_->add( Gtk::Action::create("PluginsMenu", "_Plugins") );
    action_group_->add( Gtk::Action::create("HelpMenu", "_Help") );

    action_group_->add( Gtk::Action::create("New",    Gtk::Stock::NEW,   "_New",        "Create a new file"),	sigc::mem_fun(doc_manager_, &DocManager::create_new_file) );
    action_group_->add( Gtk::Action::create("Open",   Gtk::Stock::OPEN,  "_Open",       "Open a file"),			sigc::mem_fun(this, &MainWindowImpl::on_file_open) );
    action_group_->add( Gtk::Action::create("Save",   Gtk::Stock::SAVE,  "_Save",       "Save current file"),	sigc::mem_fun(doc_manager_, &DocManager::save_current_file) );
    action_group_->add( Gtk::Action::create("SaveAs", Gtk::Stock::SAVE,  "Save _As...", "Save to a file"),		sigc::mem_fun(this, &MainWindowImpl::on_file_save_as) );
    action_group_->add( Gtk::Action::create("Close",  Gtk::Stock::CLOSE, "_Close",      "Close current file"),	sigc::mem_fun(doc_manager_, &DocManager::close_current_file) );
    action_group_->add( Gtk::Action::create("Quit",   Gtk::Stock::QUIT,  "_Quit",       "Quit"),				sigc::mem_fun(this, &MainWindowImpl::on_file_quit) );
    action_group_->add( Gtk::Action::create("About",  Gtk::Stock::ABOUT, "About",       "About"),				sigc::mem_fun(this, &MainWindowImpl::on_help_about) );

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

void MainWindowImpl::on_help_about() {
    Gtk::MessageDialog(*this, __FUNCTION__).run();
}

