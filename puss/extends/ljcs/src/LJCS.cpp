// LJCS.cpp
// 

#include "LJCS.h"

LJCS::LJCS() : app(0) {}

LJCS::~LJCS() { destroy(); }

bool LJCS::create(Puss* _app) {
	app = _app;

	icons.create(app);
	preview_page.create(app, &env);

	GtkNotebook* doc_panel = puss_get_doc_panel(app);
	g_signal_connect(doc_panel, "page-added",   G_CALLBACK(&LJCS::on_doc_page_added),   this);
	g_signal_connect(doc_panel, "page-removed", G_CALLBACK(&LJCS::on_doc_page_removed), this);

	return true;
}

void LJCS::destroy() {
	preview_page.destroy();
	icons.destroy();

	app = 0;
}

void LJCS::do_button_release_event(GtkWidget* view, GtkTextBuffer* buf, GdkEventButton* event, cpp::File* file) {
}

gboolean LJCS::on_button_release_event(GtkWidget* view, GdkEventButton* event, gpointer tag) {
	LJCS* self = (LJCS*)tag;

    //tip_.hide_all_tip();

	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GString* url = self->app->doc_get_url(buf);

	cpp::File* file = self->env.find_parsed(std::string(url->str, url->len));
	if( file ) {
		self->do_button_release_event(view, buf, event, file);
		self->env.file_decref(file);
	}

    return FALSE;
}

void LJCS::on_doc_page_added(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, gpointer tag) {
	LJCS* self = (LJCS*)tag;

	GtkTextView* view = self->app->doc_get_view_from_page(page);
	if( !view )
		return;

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return;

	g_signal_connect(view, "button-release-event", G_CALLBACK(&LJCS::on_button_release_event), tag);

    //cons.push_back( view.signal_key_press_event().connect(		sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_key_press_event),		&page ), false ) );
    //cons.push_back( view.signal_key_release_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_key_release_event),		&page )		   ) );
    //cons.push_back( view.signal_button_release_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_button_release_event),	&page ), false ) );
    //cons.push_back( view.signal_motion_notify_event().connect(	sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_motion_notify_event),	&page )		   ) );
    //cons.push_back( view.signal_focus_out_event().connect(		sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_focus_out_event),		&page )		   ) );
    //cons.push_back( view.signal_scroll_event().connect(			sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_scroll_event),			&page )		   ) );
    //cons.push_back( view.get_buffer()->signal_modified_changed().connect( sigc::bind( sigc::mem_fun(this, &LJCSPluginImpl::on_modified_changed), &page ) ) );

	GString* url = self->app->doc_get_url(buf);
	if( url ) {
        //parse_thread_.add(url);
    }

}

void LJCS::on_doc_page_removed(GtkNotebook* doc_panel, GtkWidget* page, guint page_num, gpointer tag) {
}

