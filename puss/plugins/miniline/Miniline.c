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
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>

GdkColor FAILED_COLOR = { 0, 65535, 10000, 10000 };

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

static gboolean get_current_document_insert_pos(Miniline* miniline, gint* line, gint* offset) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	GtkTextBuffer* buf;
	GtkTextView* view;

	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	get_insert_pos(buf, line, offset);
	return TRUE;
}

static gboolean find_next_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current) {
	GtkTextIter iter, end;
	GtkSourceSearchFlags flags = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_selection_bounds(buf, &iter, &end);
	if( skip_current )
		gtk_text_iter_forward_char(&iter);
	gtk_text_buffer_get_end_iter(buf, &end);

	if( !gtk_source_iter_forward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_start_iter(buf, &iter);

		if( !gtk_source_iter_forward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static gboolean find_prev_text(GtkTextView* view, const gchar* text, GtkTextIter* ps, GtkTextIter* pe, gboolean skip_current) {
	GtkTextIter iter, end;
	GtkSourceSearchFlags flags = (GtkSourceSearchFlags)(GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE);

	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	gtk_text_buffer_get_start_iter(buf, &end);

	if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
	{
		gtk_text_buffer_get_end_iter(buf, &iter);

		if( !gtk_source_iter_backward_search(&iter, text, flags, ps, pe, &end) )
			return FALSE;
	}

	return TRUE;
}

static void find_and_locate_text(Miniline* miniline, GtkTextView* view, const gchar* text, gboolean is_forward, gboolean skip_current) {
	GtkTextIter ps, pe;
	gboolean res = is_forward
		? find_next_text(view, text, &ps, &pe, skip_current)
		: find_prev_text(view, text, &ps, &pe, skip_current);

	if( res ) {
		gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);
		gtk_text_buffer_select_range(gtk_text_view_get_buffer(view), &ps, &pe);
		gtk_text_view_scroll_to_iter(view, &ps, 0.0, FALSE, 1.0, 0.25);
	} else {
		gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, &FAILED_COLOR);
	}
}

static void fix_selected_range(GtkTextView* view) {
	GtkTextIter ps, pe;
	GtkTextBuffer* buf = gtk_text_view_get_buffer(view);
	if( buf && gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) )
		gtk_text_buffer_select_range(buf, &pe, &ps);
}

/*
//--------------------------------------------------------------
// mini line REPLACE
//--------------------------------------------------------------

typedef struct _MinilineREPLACE {
	MinilineCallback cb;
} MinilineREPLACE;

static gchar* get_replace_search_text(GtkEntry* entry) {
	gchar* p;
	gchar* text = g_strdup(gtk_entry_get_text(entry));
	for( p = text; *p ; ++p ) {
		if( *p=='/' ) {
			*p = '\0';
			break;
		}
	}
	return text;
}

static gboolean REPLACE_cb_active(Miniline* miniline, gpointer tag) {
	GtkTextBuffer* buf;
	gchar* text = 0;
	gchar* stext;
	gint page_num;
	GtkTextView* view;
	gint sel_ps = 0;
	GtkTextIter ps, pe;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view )
		return FALSE;

	buf = gtk_text_view_get_buffer(view);
	if( !buf )
		return FALSE;

	gtk_widget_modify_base(GTK_WIDGET(miniline->entry), GTK_STATE_NORMAL, NULL);

	gtk_image_set_from_stock(miniline->image, GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_SMALL_TOOLBAR);

	if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
		text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
		sel_ps = (gint)strlen(text) + 1;
	} else {
		text = g_strdup("<text>");
	}

	stext = g_strconcat(text, "/<replace>/[all]", NULL);
	gtk_entry_set_text(miniline->entry, stext);
	gtk_entry_select_region(miniline->entry, sel_ps, -1);

	g_free(text);
	g_free(stext);

	return TRUE;
}

static void REPLACE_cb_changed(Miniline* miniline, gpointer tag) {
	gchar* text;
	gint page_num;
	GtkTextView* view;

	//MinilineREPLACE* self = (MinilineREPLACE*)tag;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return;
	}

	text = get_replace_search_text(miniline->entry);

	if( *text!='\0' )
		find_and_locate_text(miniline, view, text, TRUE, FALSE);

	g_free(text);
}

static gchar* locate_next_scope(gchar* text) {
	gchar* p;
	gchar* pos = 0;
	for( p = text; *p; ++p ) {
		if( *p=='/' ) {
			*p = '\0';
			pos = p + 1;
			break;
		}
	}
	return pos;
}

static void do_replace_all( GtkTextBuffer* buf
		, const gchar* find_text
		, const gchar* replace_text
		, gint flags )
{
	gint replace_text_len;
	GtkTextIter iter, ps, pe;
	gtk_text_buffer_get_start_iter(buf, &iter);

	gtk_text_buffer_begin_user_action(buf);
	{
		replace_text_len = (gint)strlen(replace_text);

		while( gtk_source_iter_forward_search(&iter
				, find_text
				, (GtkSourceSearchFlags)flags
				, &ps
				, &pe
				, 0) )
		{
			gtk_text_buffer_delete(buf, &ps, &pe);
			gtk_text_buffer_insert(buf, &ps, replace_text, replace_text_len);
			iter = ps;
		}
	}
	gtk_text_buffer_end_user_action(buf);
}

static void replace_and_locate_text(Miniline* miniline, GtkTextView* view) {
	gchar* text;
	gchar* ops_text;
	gboolean replace_all_sign;
	GtkTextBuffer* buf;
	GtkTextIter ps, pe;

	gchar* find_text = g_strdup(gtk_entry_get_text(miniline->entry));
	gchar* replace_text = locate_next_scope(find_text);

	if( !replace_text ) {
		text = g_strconcat(find_text, "/<replace>/[all]", NULL);
		gtk_entry_set_text(miniline->entry, text);
		g_free(text);

		gtk_entry_select_region(miniline->entry, (gint)strlen(find_text), -1);

	} else {
		ops_text = locate_next_scope(replace_text);
		replace_all_sign = (ops_text && ops_text[0]=='a');

		buf = gtk_text_view_get_buffer(view);

		if( replace_all_sign ) {
			do_replace_all( buf
								, find_text
								, replace_text
								, GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE );

			miniline->deactive(miniline);

		} else {
			gtk_text_buffer_get_selection_bounds(buf, &ps, &pe);
			if( gtk_text_iter_is_end(&ps) || gtk_text_iter_is_end(&pe) ) {
				miniline->deactive(miniline);
				
			} else {
				gtk_text_buffer_begin_user_action(buf);
				gtk_text_buffer_delete(buf, &ps, &pe);
				gtk_text_buffer_insert(buf, &ps, replace_text, (gint)strlen(replace_text));
				gtk_text_buffer_end_user_action(buf);

				find_and_locate_text(miniline, view, find_text, TRUE, FALSE);
			}
		}
	}

	g_free(find_text);
}

static gboolean REPLACE_cb_key_press(Miniline* miniline, GdkEventKey* event, gpointer tag) {
	gint page_num;
	GtkTextView* view;
	const gchar* text;
	gchar* rtext;

	if( event->keyval==GDK_Escape ) {
		miniline->deactive(miniline);
		return TRUE;
	}

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(miniline->app));
	view = miniline->app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		miniline->deactive(miniline);
		return TRUE;
	}

	text = gtk_entry_get_text(miniline->entry);
	if( *text=='\0' || *text=='/' )
		return FALSE;

	if( *text!='\0' ) {
		switch( event->keyval ) {
		case GDK_Return:
			replace_and_locate_text(miniline, view);
			return TRUE;

		case GDK_Up:
		case GDK_Down:
			{
				rtext = get_replace_search_text(miniline->entry);
				find_and_locate_text(miniline, view, rtext, event->keyval==GDK_Down, TRUE);
				g_free(rtext);
			}
			return TRUE;
		}
	}

	return FALSE;
}

PUSS_EXPORT MinilineCallback* miniline_REPLACE_get_callback() {
	static MinilineREPLACE me;
	me.cb.tag = &me;
	me.cb.cb_active = &REPLACE_cb_active;
	me.cb.cb_changed = &REPLACE_cb_changed;
	me.cb.cb_key_press = &REPLACE_cb_key_press;

	return &me.cb;
}
*/

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
				find_and_locate_text(g_self, view, text, TRUE, FALSE);
				miniline_switch_image( GTK_STOCK_FIND )
				break;

			default:
				gtk_widget_modify_base(GTK_WIDGET(g_self->entry), GTK_STATE_NORMAL, &FAILED_COLOR);
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

	if( event->keyval==GDK_Escape ) {
		g_self->app->doc_locate(page_num, g_self->last_line, g_self->last_offset, FALSE);
		miniline_deactive();
		return TRUE;
	}

	text = gtk_entry_get_text(g_self->entry);
	switch( *text ) {
	case '\0':
		return FALSE;

	case ':':
		++text;
	case '0':	case '1':	case '2':	case '3':	case '4':
	case '5':	case '6':	case '7':	case '8':	case '9':
		switch( event->keyval ) {
		case GDK_Return:
			miniline_deactive();
			g_self->app->doc_locate(page_num, -1, -1, TRUE);
			return TRUE;
		case GDK_Up:
		case GDK_Down:
		case GDK_Tab:
			return TRUE;
		}
		break;

	case '/':
		++text;
		switch( event->keyval ) {
		case GDK_Return:
			miniline_deactive();
			fix_selected_range(view);
			return TRUE;

		case GDK_Up:
			find_and_locate_text(g_self, view, text, FALSE, TRUE);
			return TRUE;

		case GDK_Down:
			find_and_locate_text(g_self, view, text, TRUE, TRUE);
			return TRUE;

		case GDK_Tab:
			return TRUE;
		}
		break;

	default:
		switch( event->keyval ) {
		case GDK_Return:
			miniline_deactive();
			return TRUE;
		case GDK_Up:
		case GDK_Down:
		case GDK_Tab:
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static gboolean miniline_cb_button_press_event( GtkWidget* widget, GdkEventButton* event ) {
	miniline_deactive();
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
		buf = gtk_text_view_get_buffer(view);
		if( buf ) {
			get_insert_pos(buf, &(g_self->last_line), &(g_self->last_offset));
			gtk_widget_modify_base(GTK_WIDGET(g_self->entry), GTK_STATE_NORMAL, NULL);
		
			if( gtk_text_buffer_get_selection_bounds(buf, &ps, &pe) ) {
				text = gtk_text_buffer_get_text(buf, &ps, &pe, FALSE);
				gtk_entry_set_text(g_self->entry, "/");
				gtk_entry_append_text(g_self->entry, text);
				miniline_switch_image( GTK_STOCK_FIND );
				g_free(text);

			} else {
				gtk_entry_set_text(g_self->entry, "");
				miniline_switch_image( GTK_STOCK_DIALOG_QUESTION );
			}
			gtk_entry_select_region(g_self->entry, 0, -1);
			res = TRUE;
		}
	}
	g_signal_handler_unblock(G_OBJECT(g_self->entry), g_self->signal_changed_id);

	gtk_widget_show(g_self->panel);
	gtk_im_context_focus_out( view->im_context );

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
	gtk_action_group_add_action_with_accel(group, g_self->action, "<control>K");

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
	GtkBox* hbox;

	builder = g_self->app->get_ui_builder();
	group = GTK_ACTION_GROUP( gtk_builder_get_object(builder, "main_action_group") );
	ui_mgr = GTK_UI_MANAGER( gtk_builder_get_object(builder, "main_ui_manager") );
	hbox = GTK_BOX( gtk_builder_get_object(builder, "main_toolbar_hbox") );

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


