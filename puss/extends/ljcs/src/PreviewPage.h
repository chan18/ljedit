// PreviewPage.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H
#define PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H

#include <gtk/gtk.h>
#include <string>
#include "Utils.h"
#include "ljcs/ljcs.h"

class Environ;
struct Puss;

class PreviewPage {
public:
    gboolean create(Puss* app, Environ* env);
    void destroy();

	void preview(const gchar* key, const gchar* key_text, cpp::File& file, size_t line);

private:
	struct SearchContent {
		std::string key;
		std::string key_text;
		cpp::File*  file;
		size_t      line;

		SearchContent() : file(0) {}
	};

	void search_thread();

	static gpointer search_thread_wrapper(gpointer self);

private:
    void do_preview(size_t index);
    void do_preview_impl(size_t index);
	void do_scroll_to_define_line();

	static gboolean search_result_update_timeout_wrapper(gpointer self);
	static gboolean scroll_to_define_line_wrapper(gpointer self);

private:
	Puss*					app_;
	Environ*				env_;

	// UI
	GtkWidget*				self_panel_;
	GtkLabel*				filename_label_;
	GtkButton*				number_button_;
	GtkTextView*			preview_view_;

	// search thread
	GThread*				search_thread_;
	Mutex<SearchContent>	search_content_;
	Cond					search_run_sem_;
	volatile gboolean		search_stopsign_;

	// preview result
    volatile gboolean		search_resultsign_;
    RWLock<cpp::Elements>	elems_;
	size_t					index_;
	cpp::File*				last_preview_file_;
};

#endif//PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H

