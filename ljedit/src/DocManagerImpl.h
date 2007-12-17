// DocManagerImpl.h
// 

#ifndef LJED_INC_DOCMANAGERIMPL_H
#define LJED_INC_DOCMANAGERIMPL_H

#include "DocManager.h"
#include "DocPageImpl.h"


struct PosNode {
	DocPageImpl*	page;
	int				line;
	int				lpos;

	PosNode*		next;
	PosNode*		prev;
};

class DocManagerImpl : public DocManager {
public:
	DocManagerImpl(Gtk::Window& parent);
    ~DocManagerImpl();

public:
	virtual DocPage& child_to_page(Gtk::Widget& child)
		{ return ((DocPageImpl&)child); }

	void create(const std::string& path);

    virtual void create_new_file();
	virtual void show_open_dialog();
    virtual bool open_file(const std::string& filepath, int line=0, int line_offset=0);
    virtual bool locate_file(const std::string& filepath, int line=0, int line_offset=0);
    virtual void save_current_file();
	virtual void save_current_file_as();
    virtual void close_current_file();
    virtual void save_all_files();
    virtual bool close_all_files();

	DocPageImpl* get_current_doc_page()
		{ return pages().empty() ? 0 : (DocPageImpl*)(get_current()->get_child()); }

	void pos_add(DocPageImpl& page, int line, int line_offset);

	void pos_forward();
	void pos_back();

protected:
    bool save_page(DocPageImpl& page, bool is_save_as);
    bool open_page(const std::string filepath
        , const Glib::ustring& displaty_name
        , const Glib::ustring* text = 0
        , int line=0
		, int line_offset=0);
    bool close_page(DocPageImpl& page);

    void locate_page_line(int page_num, int line, int line_offset, bool record_pos=true);

    bool scroll_to_file_pos();

	bool do_locate_file(const std::string& abspath, int line=0, int line_offset=0);

private:
    void on_doc_modified_changed(DocPageImpl* page);
	bool on_page_label_button_press(GdkEventButton* event, DocPageImpl* page);
	void on_page_removed(Gtk::Widget* widget, guint page_num);

private:
	Gtk::Window&	parent_;

private:	// for options
	void on_option_changed(const std::string& id, const std::string& value, const std::string& old);

	bool		option_use_mouse_double_click_;
	std::string	option_font_name_;
	int			option_tab_width_;

private:
	GtkSourceStyleScheme* default_scheme_;

private:	// for locate_page_line
    int		locate_page_num_;
    int		locate_line_num_;
    int		locate_line_offset_;
	bool	locate_record_pos_;

private:	// for pos forward & back
	void pos_pool_init();

	PosNode*				pos_cur_;
	PosNode*				pos_first_;
	std::vector<PosNode*>	pos_nodes_;

	PosNode					_pos_pool_[128];
};

#endif//LJED_INC_DOCMANAGERIMPL_H

