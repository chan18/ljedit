// MainWindowImpl.cpp
// 

#include "MainWindowImpl.h"

#include "RuntimeException.h"


MainWindowImpl::MainWindowImpl()
{
}

MainWindowImpl::MainWindowImpl(GtkWindow* widget, Glib::RefPtr<Gnome::Glade::Xml> xml)
	: MainWindow(widget)
{
}

MainWindowImpl::~MainWindowImpl() {
}

MainWindowImpl* MainWindowImpl::create__(const std::string& path) {
	Glib::RefPtr<Gnome::Glade::Xml> xml = Gnome::Glade::Xml::create(path + "/conf/main.glade");
	
	MainWindowImpl* mw;

	xml->get_widget_derived("MainWindow", mw);
	xml->get_widget_derived("MainWindow.DocManager", mw->doc_manager__);

	//mw->create(path);
	return mw;
}

void MainWindowImpl::create(const std::string& path) {
    // create main-ui
    create_ui_manager(path + "/conf/menu.xml");

    // menubar
    Gtk::Widget* menubar = ui_manager_->get_widget("/MenuBar");
    if( menubar==0 )
        throw RuntimeException("error : create main window not find MenuBar!");
    vbox_.pack_start(*menubar, Gtk::PACK_SHRINK);

    // toolbar
    Gtk::Widget* toolbar = ui_manager_->get_widget("/ToolBar");
    if( toolbar==0 )
        throw RuntimeException("error : create main window not find ToolBar!");
    vbox_.pack_start(*toolbar, Gtk::PACK_SHRINK);

    // vpaned
    create_vpaned();
    vbox_.pack_start(vpaned_);

    // statusbar
    vbox_.pack_start(status_bar_, false, false);

    this->add(vbox_);
    this->resize(1024, 768);

    show_all();
}

void MainWindowImpl::destroy() {
	doc_manager_.close_all_files();
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

void MainWindowImpl::create_vpaned() {
    // hpaned_left
    create_vpaned_hpaneds();
    vpaned_.add1(hpaned_right_);

    // bottom panel
    create_vpaned_bottom_panel();
    vpaned_.add2(bottom_panel_);

    vpaned_.set_position(500);
}

void MainWindowImpl::create_vpaned_hpaneds() {
    // left
    hpaned_left_.set_border_width(3);

    left_panel_.set_tab_pos(Gtk::POS_BOTTOM);
    hpaned_left_.add1(left_panel_);

    doc_manager_.set_scrollable();
    hpaned_left_.add2(doc_manager_);

    // right
    hpaned_right_.set_border_width(3);
    hpaned_right_.set_position(800);

    hpaned_right_.add1(hpaned_left_);

    right_panel_.set_tab_pos(Gtk::POS_BOTTOM);
    hpaned_right_.add2(right_panel_);
}

void MainWindowImpl::create_vpaned_bottom_panel() {
    bottom_panel_.set_size_request(-1, 200);
    bottom_panel_.set_tab_pos(Gtk::POS_BOTTOM);
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

