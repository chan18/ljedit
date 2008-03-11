// LJCS.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_H
#define PUSS_EXTEND_INC_LJCS_H

#include "IPuss.h"
#include "Environ.h"
#include "Icons.h"
#include "PreviewPage.h"

class LJCS {
public:
	Puss*		app;

	Environ		env;
	Icons		icons;
	PreviewPage	preview_page;

public:
	LJCS();
	~LJCS();

    bool create(Puss* _app);
    void destroy();

private:
	void do_button_release_event(GtkWidget* view, GtkTextBuffer* buf, GdkEventButton* event, cpp::File* file);

private:
	static gboolean on_button_release_event(GtkWidget* view, GdkEventButton* event, gpointer tag);

	static void on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, gpointer tag);
	static void on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, gpointer tag);
};

#endif//PUSS_EXTEND_INC_LJCS_H

