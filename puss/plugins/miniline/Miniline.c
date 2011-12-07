// MinilineModules.c
// 


#include <libintl.h>

#define TEXT_DOMAIN "plugin_miniline"

#define _(str) dgettext(TEXT_DOMAIN, str)

#include "IPuss.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>

#if GTK_MAJOR_VERSION==2
	#include <gtksourceview/gtksourceiter.h>
	
	static const GtkSourceSearchFlags SEARCH_FLAGS = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	#define GDK_KEY_Return		GDK_Return
	#define GDK_KEY_Escape		GDK_Escape
	#define GDK_KEY_Up			GDK_Up
	#define GDK_KEY_Down		GDK_Down
	#define GDK_KEY_Tab			GDK_Tab
	#define GDK_KEY_KP_Enter	GDK_KP_Enter

#else
	static const GtkTextSearchFlags SEARCH_FLAGS = (GtkTextSearchFlags)(GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE);
#endif

static GdkColor FAILED_COLOR = { 0, 65535, 10000, 10000 };

typedef struct {
	Puss*			app;

	GtkWidget*		panel;
	GtkImage*		image;
	GtkEntry*		entry;
	GtkAction*		action;

	guint			ui_mgr_id;
	gulong			signal_changed_id;
	gulong			signal_key_press_id;

	gint			last_line;
	gint			last_offset;

	const gchar*	image_stock;

	gchar*			last_search_text;
} Miniline;

static Miniline* g_self = 0;

#define miniline_switch_image(stock) \
	if( g_self->image_stock != (stock) ) { \
		g_self->image_stock = (stock); \
		gtk_image_set_from_stock(g_self->image, g_self->image_stock, GTK_ICON_SIZE_SMALL_TOOLBAR); \
	}

static void get_insert_pos(GtkTextBuffer* buf, gint* line, gint* offset) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	*line = gtk_text_iter_get_line(&iter);
	*offset = gtk_text_iter_get_line_offset(&iter);
}

static void find_and_locate_text(Miniline* miniline, GtkTextView* view, const gchar* text, gboolean is_forward, gboolean skip_current) {
	if( g_self->app->find_and_locate_text(view, text, is_forward, skip_current, TRUE, TRUE, TRUE, SEARCH_FLAGS) )
		gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);
	else
		gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, &FAILED_COLOR);
}

static void select_current_search(GtkTextView* view) {
	GtkTextIter ps, pe;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	GtkTextMark* ms = gtk_text_buffer_get_mark(buf, "puss:searched_mark_start");
	GtkTextMark* me = gtk_text_buffer_get_mark(buf, "puss:searched_mark_end");
	if( buf && ms && me ) {
		gtk_text_buffer_get_iter_at_mark(buf, &ps, ms);
		gtk_text_buffer_get_iter_at_mark(buf, &pe, me);
		gtk_text_buffer_remove_tag_by_name(buf, "puss:searched_current", &ps, &pe);
		gtk_text_buffer_select_range(buf, &ps, &pe);
	}
}

static void miniline_deactive() {
	gint page_num;
	GtkTextView* view;
	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(g_self->app));
	view = g_self->app->doc_get_view_from_page_num(page_num);
	gtk_widget_hide(g_self->panel);
	gtk_widget_grab_focus(GTK_WIDGET(view));
}

static void miniline_cb_changed( GtkEditable* editable ) {
	g_signal_handler_block(G_OBJECT(g_self->entry), g_self->signal_changed_id);
	{
		const gchar* text;
		gint page_num;
		gint line;
		GtkTextView* view;

		page_num = gtk_notebook_get_current_page(puss_get_doc_panel(g_self->app));
		view = g_self->app->doc_get_view_from_page_num(page_num);
		if( !view ) {
			miniline_deactive();

		} else {
			text = gtk_entry_get_text(g_self->entry);
			switch( *text ) {
			case '\0':
				g_self->app->doc_locate(page_num, g_self->last_line, g_self->last_offset, FALSE);
				miniline_switch_image( GTK_STOCK_DIALOG_QUESTION );
				break;

			case ':':
				++text;
			case '0':	case '1':	case '2':	case '3':	case '4':
			case '5':	case '6':	case '7':	case '8':	case '9':
				line = atoi(text) - 1;
				g_self->app->doc_locate(page_num
					, (line < 0) ? g_self->last_line : line
					, g_self->last_offset
					, FALSE);
				gtk_widget_modify_base( GTK_WIDGET(g_self->entry)
					, GTK_STATE_NORMAL
					, (gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(view)) > line) ? NULL : &FAILED_COLOR);
				miniline_switch_image( GTK_STOCK_JUMP_TO );
				break;

			case '/':
				++text;
				// no break;

			default:
				find_and_locate_text(g_self, view, text, TRUE, FALSE);
				miniline_switch_image( GTK_STOCK_FIND );
				break;
			}
		}
	}
	g_signal_handler_unblock(G_OBJECT(g_self->entry), g_self->signal_changed_id);
}

static gboolean miniline_cb_focus_out_event( GtkWidget* widget, GdkEventFocus* event ) {
	gtk_widget_hide(g_self->panel);
	return FALSE;
}

static gboolean miniline_cb_key_press_event( GtkWidget* widget, GdkEventKey* event ) {
	const gchar* text;
	gint page_num;
	GtkTextView* view;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(g_self->app));
	view = g_self->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline_deactive();
		return TRUE;
	}

	text = gtk_entry_get_text(g_self->entry);
	switch( *text ) {
	case ':':
		++text;
	case '0':	case '1':	case '2':	case '3':	case '4':
	case '5':	case '6':	case '7':	case '8':	case '9':
		switch( event->keyval ) {
		case GDK_KEY_Return:
			miniline_deactive();
			g_self->app->doc_locate(page_num, -1, -1, TRUE);
			return TRUE;

		case GDK_KEY_Escape:
			g_self->app->doc_locate(page_num, g_self->last_line, g_self->last_offset, FALSE);
			g_self->app->find_and_locate_text(view, NULL, TRUE, TRUE, TRUE, TRUE, FALSE, SEARCH_FLAGS);
			miniline_deactive();
			return TRUE;

		case GDK_KEY_Up:
		case GDK_KEY_Down:
		case GDK_KEY_Tab:
			return TRUE;
		}
		break;

	case '/':
		++text;
		// no break;

	default:
		switch( event->keyval ) {
		case GDK_KEY_Return:
		case GDK_KEY_KP_Enter:
			miniline_deactive();
			select_current_search(view);
			return TRUE;

		case GDK_KEY_Escape:
			g_self->app->doc_locate(page_num, g_self->last_line, g_self->last_offset, FALSE);
			g_self->app->find_and_locate_text(view, NULL, TRUE, TRUE, TRUE, TRUE, FALSE, SEARCH_FLAGS);
			miniline_deactive();
			return TRUE;

		case GDK_KEY_Up:
			find_and_locate_text(g_self, view, text, FALSE, TRUE);
			return TRUE;

		case GDK_KEY_Down:
			find_and_locate_text(g_self, view, text, TRUE, TRUE);
			return TRUE;

		case GDK_KEY_Tab:
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static void miniline_cb_active(GtkAction* action) {
	gboolean res = FALSE;
	gint page_num;
	GtkTextView* view;
	GtkWidget* actived;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(g_self->app));
	if( page_num < 0 )
		return;

	view = g_self->app->doc_get_view_from_page_num(page_num);
	actived = gtk_window_get_focus(puss_get_main_window(g_self->app));
	if( GTK_WIDGET(view)!=actived )
		gtk_widget_grab_focus(GTK_WIDGET(view));

	g_signal_handler_block(G_OBJECT(g_self->entry), g_self->signal_changed_id);
	{
		GtkTextBuffer* buf;
		GtkTextIter ps, pe;
		gchar* text;
		gchar* ftext;
		buf = gtk_text_view_get_buffer(view);
		if( buf ) {
			get_insert_pos(buf, &(g_self->last_line), &(g_self->last_offset));
			gtk_widget_modify_base(GTK_WIDGET(g_self->entry), GTK_STATE_NORMAL, NULL);

			if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
				text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
				ftext = g_strdup_printf("/%s", text);
				gtk_entry_set_text(g_self->entry, ftext);
				g_free(ftext);
				miniline_switch_image( GTK_STOCK_FIND );
				g_self->app->find_and_locate_text(view, text, TRUE, FALSE, TRUE, TRUE, FALSE, SEARCH_FLAGS);
				g_free(text);

			} else {
				const gchar* old_value = gtk_entry_get_text(g_self->entry);
				if( old_value && old_value[0] ) {
					
				} else {
					gtk_entry_set_text(g_self->entry, "");
					miniline_switch_image( GTK_STOCK_DIALOG_QUESTION );
					g_self->app->find_and_locate_text(view, 0, TRUE, FALSE, TRUE, TRUE, FALSE, SEARCH_FLAGS);
				}
			}
#if GTK_MAJOR_VERSION==2
			gtk_entry_select_region(g_self->entry, 0, -1);
#endif
			res = TRUE;
		}
	}
	g_signal_handler_unblock(G_OBJECT(g_self->entry), g_self->signal_changed_id);

	gtk_widget_show(g_self->panel);

#if GTK_MAJOR_VERSION==2
	gtk_im_context_focus_out( view->im_context );
#endif

	gtk_widget_grab_focus(GTK_WIDGET(g_self->entry));

	if( !res )
		miniline_deactive();
}

const gchar* MINILINE_MENU =
    "<ui>"
    "	<menubar name='main_menubar'>"
    "	  <menu action='edit_menu'>"
    "	    <placeholder name='edit_menu_plugins_place'>"
    "	      <menuitem action='plugin_miniline_action'/>"
    "	    </placeholder>"
    "	  </menu>"
    "	</menubar>"
    "</ui>"
	;

const gchar* MINILINE_UI =
    "<interface>"
    "<object class='GtkHBox' id='mini_bar_panel'>"
    "  <child>"
    "    <object class='GtkImage' id='mini_bar_image'>"
    "    </object>"
    "    <packing>"
    "      <property name='expand'>false</property>"
    "      <property name='padding'>3</property>"
    "      <property name='position'>0</property>"
    "    </packing>"
    "  </child>"
    "  <child>"
    "    <object class='GtkEntry' id='mini_bar_entry'>"
	"      <property name='has-tooltip'>true</property>"
    "    </object>"
    "    <packing>"
    "      <property name='expand'>false</property>"
    "      <property name='position'>1</property>"
    "    </packing>"
    "  </child>"
    "</object>"
    "</interface>"
	;

const gchar* HELP_TEXT =
	"<line>    -  goto line.\n"
	"<text>    -  find text.\n"
	":<line>   -  goto line.\n"
	"/<text>   -  find text.\n"
	;

static void miniline_create() {
	GtkActionGroup* group;
	GtkBuilder* builder;
	GtkUIManager* ui_mgr;
	GtkBox* hbox;

	g_self->action = gtk_action_new("plugin_miniline_action", _("miniline"), _("active miniline."), GTK_STOCK_FIND);
	g_signal_connect(g_self->action, "activate", G_CALLBACK(miniline_cb_active), 0);

	builder = g_self->app->get_ui_builder();
	group = GTK_ACTION_GROUP( gtk_builder_get_object(builder, "main_action_group") );
	ui_mgr = GTK_UI_MANAGER( gtk_builder_get_object(builder, "main_ui_manager") );
	hbox = GTK_BOX( gtk_builder_get_object(builder, "main_toolbar_hbox") );
	gtk_action_group_add_action_with_accel(group, g_self->action, "<control>G");

	g_self->ui_mgr_id = gtk_ui_manager_add_ui_from_string(ui_mgr, MINILINE_MENU, -1, 0);
	gtk_ui_manager_ensure_update(ui_mgr);

	builder = gtk_builder_new();
	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);
	gtk_builder_add_from_string(builder, MINILINE_UI, -1, 0);
	g_self->panel = GTK_WIDGET( gtk_builder_get_object(builder, "mini_bar_panel") );
	g_self->entry = GTK_ENTRY( gtk_builder_get_object(builder, "mini_bar_entry") );
	gtk_widget_set_tooltip_text( GTK_WIDGET(g_self->entry), _(HELP_TEXT) );
	g_self->image = GTK_IMAGE( gtk_builder_get_object(builder, "mini_bar_image") );
	miniline_switch_image( GTK_STOCK_DIALOG_QUESTION );

	g_self->signal_changed_id = g_signal_connect(g_self->entry, "changed", G_CALLBACK(miniline_cb_changed), 0);
	g_self->signal_key_press_id = g_signal_connect(g_self->entry, "key-press-event", G_CALLBACK(miniline_cb_key_press_event), 0);
	g_signal_connect(g_self->entry, "focus-out-event", G_CALLBACK(miniline_cb_focus_out_event), 0);

	gtk_widget_show_all(g_self->panel);
	gtk_widget_hide(g_self->panel);

	gtk_box_pack_end(hbox, g_self->panel, FALSE, FALSE, 0);
	g_object_unref(builder);
}

static void miniline_destroy() {
	GtkBuilder* builder;
	GtkUIManager* ui_mgr;
	GtkActionGroup* group;
	// GtkBox* hbox;

	builder = g_self->app->get_ui_builder();
	group = GTK_ACTION_GROUP( gtk_builder_get_object(builder, "main_action_group") );
	ui_mgr = GTK_UI_MANAGER( gtk_builder_get_object(builder, "main_ui_manager") );
	// hbox = GTK_BOX( gtk_builder_get_object(builder, "main_toolbar_hbox") );

	gtk_ui_manager_remove_ui(ui_mgr, g_self->ui_mgr_id);
	gtk_action_group_remove_action(group, g_self->action);
	g_object_unref(g_self->action);
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	g_self = (Miniline*)g_new0(Miniline, 1);
	g_self->app = app;

	miniline_create();

	return 0;
}

PUSS_EXPORT void  puss_plugin_destroy(void* ext) {
	miniline_destroy();

	g_free(g_self);
	g_self = 0;
}


