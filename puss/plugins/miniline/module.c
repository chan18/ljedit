// module.cpp
//

#include <libintl.h>

#define TEXT_DOMAIN "plugin_miniline"

#define _(str) dgettext(TEXT_DOMAIN, str)

#include "IMiniLine.h"

#include <memory.h>
#include <stdlib.h>

#define puss_get_mini_window_image(app)	GTK_IMAGE(puss_get_ui_object((app), "mini_bar_image"))
#define puss_get_mini_window_entry(app)	GTK_ENTRY(puss_get_ui_object((app), "mini_bar_entry"))

typedef struct {
	MiniLine			_parent;

	gulong				signal_id_changed;
	gulong				signal_id_key_press;

	MiniLineCallback*	cb;
} MiniLineImpl;

#define SELF_PRIV	((MiniLineImpl*)self)

static void miniline_active(MiniLine* self, MiniLineCallback* cb);
static void miniline_deactive(MiniLine* self);

SIGNAL_CALLBACK void miniline_cb_changed( GtkEditable* editable, MiniLine* self ) {
	if( SELF_PRIV->cb ) {
		g_signal_handler_block(G_OBJECT(self->entry), SELF_PRIV->signal_id_changed);
		SELF_PRIV->cb->cb_changed(self, SELF_PRIV->cb->tag);
		g_signal_handler_unblock(G_OBJECT(self->entry), SELF_PRIV->signal_id_changed);
	}
}

SIGNAL_CALLBACK gboolean miniline_cb_focus_out_event( GtkWidget* widget, GdkEventFocus* event, MiniLine* self ) {
	gtk_widget_hide(self->window);
	return FALSE;
}

SIGNAL_CALLBACK gboolean miniline_cb_key_press_event( GtkWidget* widget, GdkEventKey* event, MiniLine* self ) {
	return SELF_PRIV->cb
		? SELF_PRIV->cb->cb_key_press(self, event, SELF_PRIV->cb->tag)
		: FALSE;
}

SIGNAL_CALLBACK gboolean miniline_cb_button_press_event( GtkWidget* widget, GdkEventButton* event, MiniLine* self ) {
	miniline_deactive(self);
	return FALSE;
}

static void miniline_active( MiniLine* self, MiniLineCallback* cb ) {
	gboolean res;
	gint page_num;
	GtkTextView* view;
	GtkWidget* actived;
	SELF_PRIV->cb = cb;
	if( !cb )
		return;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	view = self->app->doc_get_view_from_page_num(page_num);
	actived = gtk_window_get_focus(puss_get_main_window(self->app));
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	gtk_widget_show(self->window);
	gtk_im_context_focus_out( view->im_context );

	gtk_widget_grab_focus(GTK_WIDGET(self->entry));

	g_signal_handler_block(G_OBJECT(self->entry), SELF_PRIV->signal_id_changed);
	res = SELF_PRIV->cb->cb_active(self, SELF_PRIV->cb->tag);
	g_signal_handler_unblock(G_OBJECT(self->entry), SELF_PRIV->signal_id_changed);
	if( !res )
		miniline_deactive(self);
}

static void miniline_deactive(MiniLine* self) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	GtkTextView* view = self->app->doc_get_view_from_page_num(page_num);

	gtk_widget_hide(self->window);

	gtk_widget_grab_focus(GTK_WIDGET(view));
}

static void miniline_build_ui(MiniLine* self) {
	GtkBuilder* builder;
	GError* err = 0;
	gchar* filepath;
	GtkBox* toolbar_hbox;

	builder = gtk_builder_new();
	if( !builder )
		return;

	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);
	filepath = g_build_filename(self->app->get_module_path(), "res", "puss_miniline.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(miniline) : build miniline ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(miniline): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	self->window = GTK_WIDGET(gtk_builder_get_object(builder, "mini_bar_window"));
	self->image = GTK_IMAGE(gtk_builder_get_object(builder, "mini_bar_image"));
	self->entry = GTK_ENTRY(gtk_builder_get_object(builder, "mini_bar_entry"));
	gtk_widget_hide(self->window);
	g_assert( self->window
		&& self->image
		&& self->entry );

	toolbar_hbox = GTK_BOX(puss_get_ui_object(self->app, "main_toolbar_hbox"));
	gtk_box_pack_end(toolbar_hbox, self->window, FALSE, FALSE, 0); 

	SELF_PRIV->signal_id_changed   = g_signal_handler_find(self->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0, (gpointer)&miniline_cb_changed, 0);
	SELF_PRIV->signal_id_key_press = g_signal_handler_find(self->entry, (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA), 0, 0, 0, (gpointer)&miniline_cb_key_press_event, 0);

	gtk_builder_connect_signals(builder, self);
	g_object_unref(G_OBJECT(builder));
}

// TODO : Put these into Plugin Layer
// 
PUSS_EXPORT MiniLineCallback* miniline_GOTO_get_callback();
PUSS_EXPORT MiniLineCallback* miniline_FIND_get_callback();
PUSS_EXPORT MiniLineCallback* miniline_REPLACE_get_callback();

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	MiniLine* self;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = (MiniLine*)g_new0(MiniLineImpl, 1);
	self->app = app;
	self->active = miniline_active;
	self->deactive = miniline_deactive;

	miniline_build_ui(self);

	return self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* ext) {
	MiniLineImpl* self = (MiniLineImpl*)ext;
	g_free(self);
}

