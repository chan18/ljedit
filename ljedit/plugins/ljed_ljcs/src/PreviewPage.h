// PreviewPage.h
// 

#ifndef LJED_INC_PREVIEWPAGE_H
#define LJED_INC_PREVIEWPAGE_H

#include "LJEditor.h"

#include "LJCSEnv.h"


class PreviewPage {
public:
    Gtk::Widget& get_widget() { return vbox_; }

    void create();
    void destroy();

public:
    PreviewPage(LJEditor& editor);
    virtual ~PreviewPage();

	void preview(const std::string& key, const std::string& key_text, cpp::File& file, size_t line);

private:
    void do_preview(size_t index=0);
    void do_preview_impl(size_t index=0);

private:
	void on_number_btn_clicked();
	bool on_number_btn_release_event(GdkEventButton* event);
	void on_number_menu_selected(size_t index);
	void on_filename_btn_clicked();
    bool on_scroll_to_define_line();
	bool on_sourceview_button_release_event(GdkEventButton* event);

private:
    LJEditor&					editor_;

private:	// thread
	void search_thread();

	struct SearchContent {
		std::string key;
		std::string key_text;
		cpp::File*  file;
		size_t      line;

		SearchContent() : file(0) {}
	};

	Glib::Thread*				search_thread_;
	Mutex<SearchContent>		search_content_;
	Glib::Cond					search_run_sem_;

    bool						search_stopsign_;
    bool						search_resultsign_;

private:	// timeout
	bool on_update_timeout();

private:
	Gtk::VBox					vbox_;

	Gtk::Menu					number_menu_;
	Gtk::Button*				number_button_;
	Gtk::Label*					filename_label_;

    gtksourceview::SourceView*	view_;

    RWLock<cpp::Elements>		elems_;
	size_t						index_;
	cpp::File*					last_preview_file_;
};

#endif//LJED_INC_PREVIEWPAGE_H

