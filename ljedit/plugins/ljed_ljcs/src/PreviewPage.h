// PreviewPage.h
// 

#ifndef LJED_INC_PREVIEWPAGE_H
#define LJED_INC_PREVIEWPAGE_H

#include "LJEditor.h"

#include "ljcs/ljcs.h"

class PreviewPage {
public:
    Gtk::Widget& get_widget() { return vbox_; }

    void create();
    void destroy();

public:
    PreviewPage(LJEditor& editor);
    virtual ~PreviewPage();

    void preview(cpp::Elements& elems, size_t index=0);

private:
	void on_number_btn_clicked();
	void on_filename_btn_clicked();
    bool on_scroll_to_define_line();
	bool on_sourceview_button_release_event(GdkEventButton* event);

private:
    LJEditor&				editor_;

private:
	Gtk::VBox				vbox_;

	Gtk::Button*			number_button_;
	Gtk::Label*				filename_label_;

    Gtk::TextView*			view_;

    cpp::Elements			elems_;
	size_t					index_;
	cpp::File*				last_preview_file_;
};

#endif//LJED_INC_PREVIEWPAGE_H

