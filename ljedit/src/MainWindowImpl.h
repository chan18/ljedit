// MainWindowImpl.h
// 

#ifndef LJED_INC_MAINWINDOWIMPL_H
#define LJED_INC_MAINWINDOWIMPL_H

#include "MainWindow.h"

#include "DocManagerImpl.h"
#include "CmdLineImpl.h"
#include "CmdLineCallbacks.h"

class MainWindowImpl : public MainWindow {
public:
	MainWindowImpl();
    virtual ~MainWindowImpl();

    void create(const std::string& path);
    void destroy();

public:
    virtual Glib::RefPtr<Gtk::UIManager>	ui_manager()	{ return ui_manager_;   }
    virtual Glib::RefPtr<Gtk::ActionGroup>	action_group()	{ return action_group_; }

	virtual CmdLine&						cmd_line()		{ return cmd_line_;     }

    virtual Gtk::Notebook&					left_panel()	{ return left_panel_;   }
    virtual DocManager&						doc_manager()	{ return doc_manager_;  }
    virtual Gtk::Notebook&					right_panel()	{ return right_panel_;  }

    virtual Gtk::Notebook&					bottom_panel()	{ return bottom_panel_; }

    virtual Gtk::Statusbar&					status_bar()	{ return status_bar_;   }

private:
    void create_ui_manager(const std::string& config_file);

private:
	bool on_delete_event(GdkEventAny* event);

    void on_file_quit();

	void on_edit_find();
	void on_edit_goto();

	void on_view_fullscreen();
	void on_view_left();
	void on_view_right();
	void on_view_bottom();

	//void on_tools_options();
	//void on_tools_font();

    void on_help_about();

private:
	void bottom_panel_active_page(int page_id);

private:
    Glib::RefPtr<Gtk::UIManager>	ui_manager_;
    Glib::RefPtr<Gtk::ActionGroup>	action_group_;

	CmdLineImpl						cmd_line_;

    Gtk::Notebook					left_panel_;
    DocManagerImpl					doc_manager_;
    Gtk::Notebook					right_panel_;

    Gtk::Notebook					bottom_panel_;

    Gtk::Statusbar					status_bar_;

private:
	CmdGotoCallback					cmd_cb_goto_;
	CmdFindCallback					cmd_cb_find_;
};

#endif//LJED_INC_MAINWINDOWIMPL_H

