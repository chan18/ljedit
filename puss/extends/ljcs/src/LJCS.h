// LJCS.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_H
#define PUSS_EXTEND_INC_LJCS_H

#include "IPuss.h"
#include "Environ.h"
#include "Icons.h"
#include "ParseThread.h"

class PreviewPage;
class OutlinePage;
struct Tips;


class LJCS {
public:
	Puss*			app;

	Environ			env;
	Icons			icons;
	PreviewPage*	preview_page;
	OutlinePage*	outline_page;
	Tips*			tips;
	ParseThread		parse_thread;

public:
	LJCS();
	~LJCS();

    bool create(Puss* _app);
    void destroy();

private:
	void open_include_file(const gchar* filename, gboolean system_header, const gchar* owner_path);

	void do_button_release_event(GtkTextView* view, GtkTextBuffer* buf, GdkEventButton* event, cpp::File* file);

	void show_include_hint(gchar* filename, gboolean system_header, GtkTextView* view);
	void show_hint(GtkTextIter* it, GtkTextIter* end, char tag, GtkTextView* view);
	void locate_sub_hint(GtkTextView* view);
	void auto_complete(GtkTextView* view);

	void set_show_hint_timer(GtkTextView* view);
	void kill_show_hint_timer();
	gboolean on_show_hint_timeout(GtkTextView* view, gint tag);

private:
	static gboolean on_key_press_event(GtkTextView* view, GdkEventKey* event, LJCS* self);
	static gboolean on_key_release_event(GtkTextView* view, GdkEventKey* event, LJCS* self);

	static gboolean on_button_release_event(GtkTextView* view, GdkEventButton* event, LJCS* self);
	static gboolean on_focus_out_event(GtkTextView* view, GdkEventFocus* event, LJCS* self);
	static gboolean on_scroll_event(GtkTextView* view, GdkEventScroll* event, LJCS* self);

	static void on_modified_changed(GtkTextBuffer* buf, LJCS* self);

	static void on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self);
	static void on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self);
};

#endif//PUSS_EXTEND_INC_LJCS_H

