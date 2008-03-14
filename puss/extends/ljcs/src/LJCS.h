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

	void do_button_release_event(GtkWidget* view, GtkTextBuffer* buf, GdkEventButton* event, cpp::File* file);

private:
	static gboolean on_key_press_event(GtkWidget* view, GdkEventKey* event, LJCS* self);
	static gboolean on_key_release_event(GtkWidget* view, GdkEventKey* event, LJCS* self);

	static gboolean on_button_release_event(GtkWidget* view, GdkEventButton* event, LJCS* self);
	static gboolean on_focus_out_event(GtkWidget* view, GdkEventFocus* event, LJCS* self);
	static gboolean on_scroll_event(GtkWidget* view, GdkEventScroll* event, LJCS* self);

	static void on_modified_changed(GtkTextBuffer* buf, LJCS* self);

	static void on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self);
	static void on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, LJCS* self);
};

#endif//PUSS_EXTEND_INC_LJCS_H

