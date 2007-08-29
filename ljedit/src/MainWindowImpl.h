// MainWindowImpl.h
// 

#ifndef LJED_INC_MAINWINDOWIMPL_H
#define LJED_INC_MAINWINDOWIMPL_H

#include "MainWindow.h"

#include "DocManagerImpl.h"


class MainWindowImpl : public MainWindow {
public:
	MainWindowImpl();
    virtual ~MainWindowImpl();

    void create(const std::string& path);
    void destroy();

public:
    virtual Glib::RefPtr<Gtk::UIManager>	ui_manager()	{ return ui_manager_;   }
    virtual Glib::RefPtr<Gtk::ActionGroup>	action_group()	{ return action_group_; }

    virtual Gtk::Notebook&					left_panel()	{ return left_panel_;   }
    virtual DocManager&						doc_manager()	{ return doc_manager_;  }
    virtual Gtk::Notebook&					right_panel()	{ return right_panel_;  }

    virtual Gtk::Notebook&					bottom_panel()	{ return bottom_panel_; }

    virtual Gtk::Statusbar&					status_bar()	{ return status_bar_;   }

private:
    void create_ui_manager(const std::string& config_file);

private:
    void on_file_new();
    void on_file_open();
    void on_file_save();
    void on_file_save_as();
    void on_file_quit();

    void on_help_about();

private:
    Glib::RefPtr<Gtk::UIManager>	ui_manager_;
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;

    Gtk::Notebook					left_panel_;
    DocManagerImpl					doc_manager_;
    Gtk::Notebook					right_panel_;

    Gtk::Notebook					bottom_panel_;

    Gtk::Statusbar					status_bar_;
};

#endif//LJED_INC_MAINWINDOWIMPL_H

