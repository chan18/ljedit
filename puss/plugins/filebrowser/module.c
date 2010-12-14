// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "IPuss.h"

#include <libintl.h>

#include <gio/gio.h>

#define TEXT_DOMAIN "plugin_filebroswer"

#define _(str) dgettext(TEXT_DOMAIN, str)

#ifdef G_OS_WIN32
	// win32
#else
	// linux
#endif

typedef struct {
	Puss*				app;

	GtkBuilder*			builder;
	GVolumeMonitor*		vm;
	GtkWidget*			panel;
	GtkTreeStore*		store;
	GtkTreeModel*		model;
	GtkTreeView*		view;
	GdkPixbuf*			folder_icon;
	GdkPixbuf*			file_icon;
	gint				icon_size;
	GSList*				string_pool;
	GSList*				object_pool;
	gulong				switch_page_id;
} PussFileBrowser;

static void fill_root(PussFileBrowser* self) {
	GtkIconTheme* theme = gtk_icon_theme_get_default();
	GList* mounts = g_volume_monitor_get_mounts(self->vm);
	GList* p;
	GMount* mnt;

	GIcon* icon;
	GtkIconInfo* info;
	GdkPixbuf* pbuf;
	gchar* name;
	GFile* file;
	GtkTreeIter iter;
	GtkTreeIter subiter;

	for(p=mounts; p; p=p->next ) {
		mnt = (GMount*)(p->data);
		icon = g_mount_get_icon(mnt);
		info = icon ? gtk_icon_theme_lookup_by_gicon(theme, icon, self->icon_size, 0) : 0;
		pbuf = info ? gtk_icon_info_load_icon(info, 0) : 0;
		name = g_mount_get_name(mnt);
		file = g_mount_get_root(mnt);

		gtk_tree_store_prepend(self->store, &iter, 0);
		gtk_tree_store_set( self->store, &iter, 0, pbuf, 1, name, 2, file, -1 );

		g_object_unref(mnt);
		if( name )
			self->string_pool = g_slist_prepend(self->string_pool, name);
		if( icon )
			g_object_unref(icon);
		if( info )
			gtk_icon_info_free(info);
		if( pbuf )
			self->object_pool = g_slist_prepend(self->object_pool, pbuf);

		// append [loading] node
		if( file ) {
			gtk_tree_store_prepend(self->store, &subiter, &iter);
			gtk_tree_store_set( self->store, &subiter, 0, 0, 1, 0, 2, 0, -1 );
			self->object_pool = g_slist_prepend(self->object_pool, file);
		}
	}

	g_list_free(mounts);
}

static void fill_subs(PussFileBrowser* self, GFile* dir, GtkTreeIter* parent_iter) {
	GtkIconTheme* theme = gtk_icon_theme_get_default();
	GFileInfo* fileinfo;
	GtkIconInfo* info;
	GIcon* icon;
	gchar* name;
	GdkPixbuf* pbuf;
	GtkTreeIter iter;
	GtkTreeIter subiter;
	GFile* subfile;
	GFileEnumerator* enumerator;
	GList* dir_infos = 0;
	GList* file_infos = 0;
	GList* p;

	enumerator = g_file_enumerate_children(dir
					, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_ICON
					, G_FILE_QUERY_INFO_NONE
					, 0, 0);
	if( !enumerator )
		return;

	while( (fileinfo = g_file_enumerator_next_file(enumerator, 0, 0)) ) {
		if( g_file_info_get_file_type(fileinfo)==G_FILE_TYPE_DIRECTORY )
			dir_infos = g_list_append(dir_infos, fileinfo);
		else
			file_infos = g_list_append(file_infos, fileinfo);
	}
	g_file_enumerator_close(enumerator, 0, 0);
	g_object_unref(enumerator);

	for( p=dir_infos; p; p=p->next ) {
		fileinfo = (GFileInfo*)(p->data);
		name = g_strdup(g_file_info_get_name(fileinfo));
		icon = g_file_info_get_icon(fileinfo);
		info = icon ? gtk_icon_theme_lookup_by_gicon(theme, icon, self->icon_size, 0) : 0;
		pbuf = info ? gtk_icon_info_load_icon(info, 0) : g_object_ref(self->folder_icon);
		subfile = g_file_get_child(dir, name);
		g_object_unref(fileinfo);

		gtk_tree_store_append(self->store, &iter, parent_iter);
		gtk_tree_store_set( self->store, &iter, 0, pbuf, 1, name, 2, subfile, -1 );
		if( name )
			self->string_pool = g_slist_prepend(self->string_pool, name);
		if( info )
			gtk_icon_info_free(info);
		if( pbuf )
			self->object_pool = g_slist_prepend(self->object_pool, pbuf);
		if( subfile ) {
			// append [loading] node
			gtk_tree_store_append(self->store, &subiter, &iter);
			gtk_tree_store_set( self->store, &subiter, 0, 0, 1, 0, 2, 0, -1 );
			self->object_pool = g_slist_prepend(self->object_pool, subfile);
		}
	}

	for( p=file_infos; p; p=p->next ) {
		fileinfo = (GFileInfo*)(p->data);
		name = g_strdup(g_file_info_get_name(fileinfo));
		icon = g_file_info_get_icon(fileinfo);
		info = icon ? gtk_icon_theme_lookup_by_gicon(theme, icon, self->icon_size, 0) : 0;
		pbuf = info ? gtk_icon_info_load_icon(info, 0) : g_object_ref(self->file_icon);
		g_object_unref(fileinfo);

		gtk_tree_store_append(self->store, &iter, parent_iter);
		gtk_tree_store_set( self->store, &iter, 0, pbuf, 1, name, 2, 0, -1 );
		if( name )
			self->string_pool = g_slist_prepend(self->string_pool, name);
		if( info )
			gtk_icon_info_free(info);
		if( pbuf )
			self->object_pool = g_slist_prepend(self->object_pool, pbuf);
	}

	g_list_free(dir_infos);
	g_list_free(file_infos);
}

static void ensure_fill_subs(PussFileBrowser* self, GtkTreeIter* parent_iter) {
	gchar* sub = 0;
	GFile* dir;
	GtkTreeIter iter;

	// check & remove [loading] node
	if( !gtk_tree_model_iter_nth_child(self->model, &iter, parent_iter, 0) )
		return;
	gtk_tree_model_get(self->model, &iter, 1, &sub, -1);
	if( sub )
		return;

	// get dir GFile
	gtk_tree_model_get(self->model, parent_iter, 2, &dir, -1);
	if( !dir )
		return;

	fill_subs(self, dir, parent_iter);
	gtk_tree_store_remove(self->store, &iter);

}

static gboolean locate_to(PussFileBrowser* self, GtkTreeIter* iter, GtkTreeIter* parent, gchar** paths) {
	gchar* str;
	GFile* dir;
	gchar* name;
	GtkTreeIter parent_iter;

locate_loop:
	for( ; *paths; ++paths )
		if( *paths[0]!='\0' )
			break;
	name = *paths;
	if( !name )
		return TRUE;
	++paths;

	if( !gtk_tree_model_iter_children(self->model, iter, parent) )
		return FALSE;

	do {
		gtk_tree_model_get(self->model, iter, 1, &str, 2, &dir, -1);
		if( !str)
			continue;

		if( dir ) { 
#ifdef G_OS_WIN32
			if( parent==0 ) {
				gint res;
				gchar* pth = g_file_get_path(dir);
				pth[2] = '\0';
				res = g_ascii_strcasecmp(pth, name);
				g_free(pth);
				if( res!=0 )
					continue;

			} else {
				if( g_ascii_strcasecmp(str, name)!=0 )
					continue;
			}
#else
			if( g_str_equal(str, name)!=0 )
				continue;
#endif
			ensure_fill_subs(self, iter);
			parent_iter = *iter;
			parent = &parent_iter;
			goto locate_loop;
		}

#ifdef G_OS_WIN32
		if( g_ascii_strcasecmp(str, name)!=0 )
			continue;
#else
		if( g_str_equal(str, name)!=0 )
			continue;
#endif

		return TRUE;
	} while( gtk_tree_model_iter_next(self->model, iter) );

	return FALSE;
}

typedef struct {
	PussFileBrowser*	self;
	GtkTreePath*		path;
} ScrollTag;

static gboolean scroll_to_path(gpointer data) {
	ScrollTag* tag = (ScrollTag*)data;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(tag->self->view);
	gtk_tree_selection_select_path(selection, tag->path);
	gtk_tree_view_scroll_to_cell(tag->self->view, tag->path, 0, FALSE, 0.5f, 0.0f);
	g_slice_free(ScrollTag, tag);
	return FALSE;
}

static void clear_filetree(PussFileBrowser* self) {
	gtk_tree_store_clear(self->store);

	g_slist_foreach(self->string_pool, (GFunc)g_free, 0);
	g_slist_free(self->string_pool);
	self->string_pool = 0;

	g_slist_foreach(self->object_pool, (GFunc)g_object_unref, 0);
	g_slist_free(self->object_pool);
	self->object_pool = 0;
}

static void locate_to_file(PussFileBrowser* self, GString* filepath, gboolean refresh_if_failed) {
	GtkTreeIter iter;
	gchar** paths;

	if( !filepath )
		return;

#ifdef G_OS_WIN32
	paths = g_strsplit_set(filepath->str, "\\/", -1);
#else
	paths = g_strsplit(filepath->str, "/", -1);
#endif

	if( locate_to(self, &iter, 0, paths) ) {
		ScrollTag* tag = g_slice_new(ScrollTag);
		GtkTreePath* path = gtk_tree_model_get_path(self->model, &iter);
		gtk_tree_view_expand_to_path(self->view, path);

		tag->path = path;
		tag->self = self;
		g_idle_add(scroll_to_path, tag);

	} else if( refresh_if_failed ) {
		GFile* file = g_file_new_for_path(filepath->str);
		if( file ) {
			if( g_file_query_exists(file, 0) ) {
				clear_filetree(self);
				fill_root(self);
				locate_to_file(self, filepath, FALSE);
			}
			g_object_unref(file);
		}
	}
	
	g_strfreev(paths);
}

static void build_ui(PussFileBrowser* self) {
	gint w, h;
	GtkBuilder* builder;
	gchar* ui_pth;

	builder = gtk_builder_new();
	ui_pth = g_build_filename(self->app->get_plugins_path(), "filebrowser.ui", NULL);
	gtk_builder_add_from_file(builder, ui_pth, 0);
	g_free(ui_pth);

	self->builder = builder;
	self->panel = GTK_WIDGET(gtk_builder_get_object(builder, "main_vbox"));
	self->store = GTK_TREE_STORE(gtk_builder_get_object(builder, "file_tree_store"));
	self->model = GTK_TREE_MODEL(self->store);
	self->view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "file_treeview"));
	self->vm = g_volume_monitor_get();
	self->folder_icon = gtk_widget_render_icon(GTK_WIDGET(self->view), GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU, 0);
	self->file_icon = gtk_widget_render_icon(GTK_WIDGET(self->view), GTK_STOCK_FILE, GTK_ICON_SIZE_MENU, 0);
	if( gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &w, &h) )
		self->icon_size = w;
	gtk_builder_connect_signals(builder, self);
	gtk_widget_show_all(self->panel);

	fill_root(self);
}

SIGNAL_CALLBACK void filebrowser_cb_refresh_button_clicked(GtkWidget* w, gpointer user_data) {
	PussFileBrowser* self = (PussFileBrowser*)user_data;
	GtkNotebook* doc_panel = puss_get_doc_panel(self->app);
	gint page_num = gtk_notebook_get_current_page(doc_panel);
	GtkTextBuffer* buf = (page_num < 0) ? 0 : self->app->doc_get_buffer_from_page_num(page_num);

	clear_filetree(self);
	fill_root(self);

	if( buf )
		locate_to_file(self, self->app->doc_get_url(buf), FALSE);
}

SIGNAL_CALLBACK void filebrowser_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* column, gpointer user_data) {
	PussFileBrowser* self = (PussFileBrowser*)user_data;
	GtkTreeIter iter;
	GFile* dir;
	const char* name;

	if( !gtk_tree_model_get_iter(self->model, &iter, path) )
		return;

	gtk_tree_model_get(self->model, &iter, 1, &name, 2, &dir, -1);
	if( !dir ) {
		// open file use puss
		gchar* pth;
		gchar* filename;
		GtkTreeIter parent_iter;
		if( !gtk_tree_model_iter_parent(self->model, &parent_iter, &iter) )
			return;

		gtk_tree_model_get(self->model, &parent_iter, 2, &dir, -1);
		if( !dir )
			return;

		pth = g_file_get_path(dir);
		filename = g_build_filename(pth, name, NULL);
		self->app->doc_open(filename, -1, -1, TRUE);
		g_free(filename);
		g_free(pth);

	} else if( gtk_tree_view_row_expanded(tree_view, path) ) {
		gtk_tree_view_collapse_row(tree_view, path);

	} else {
		gtk_tree_view_expand_to_path(tree_view, path);
	}
}

SIGNAL_CALLBACK void filebrowser_cb_row_collapsed(GtkTreeView* tree_view, GtkTreeIter* iter, GtkTreePath* path, gpointer user_data) {
	// TODO free sub tree & add [loading] node
}

SIGNAL_CALLBACK void filebrowser_cb_row_expanded(GtkTreeView* tree_view, GtkTreeIter* iter, GtkTreePath* path, gpointer user_data) {
	PussFileBrowser* self = (PussFileBrowser*)user_data;
	ensure_fill_subs(self, iter);
}

SIGNAL_CALLBACK gboolean filebrowser_view_cb_keypress(GtkWidget* w, GdkEventKey* event, gpointer user_data) {
	PussFileBrowser* self = (PussFileBrowser*)user_data;

	switch( event->keyval ) {
	case GDK_KEY_Left: {
			GtkTreePath* path;
			gtk_tree_view_get_cursor(self->view, &path, 0);
			if( path && gtk_tree_view_row_expanded(self->view, path) ) {
				gtk_bindings_activate((GtkObject*)w, GDK_KEY_Return, 0);
			} else {
				gtk_bindings_activate((GtkObject*)w, GDK_KEY_BackSpace, 0);
			}
			g_signal_stop_emission_by_name(w, "key-press-event");
			return TRUE;
		}
		break;
	case GDK_KEY_Right: {
			gtk_bindings_activate((GtkObject*)w, GDK_KEY_Return, 0);
			g_signal_stop_emission_by_name(w, "key-press-event");
			return TRUE;
		}
		break;
	default:
		break;
	}

	return FALSE;
}

static void on_switch_page(GtkNotebook* nb, GtkNotebookPage* page, guint page_num, gpointer user_data) {
	PussFileBrowser* self = (PussFileBrowser*)user_data;
	GtkTextBuffer* buf = self->app->doc_get_buffer_from_page_num(page_num);
	GString* filepath;
	if( !buf )
		return;

	filepath = self->app->doc_get_url(buf);
	if( !filepath )
		return;

	locate_to_file(self, filepath, TRUE);
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	PussFileBrowser* self;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(PussFileBrowser, 1);
	self->app = app;
	build_ui(self);

	self->app->panel_append(self->panel
				, gtk_label_new(_("FileBrowser"))
				, "puss_file_browser_plugin_panel"
				, PUSS_PANEL_POS_LEFT);

	self->switch_page_id = g_signal_connect(puss_get_doc_panel(app), "switch-page", (GCallback)on_switch_page, self);

	return self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* ext) {
	PussFileBrowser* self = (PussFileBrowser*)ext;

	clear_filetree(self);

	self->app->panel_remove(self->panel);

	g_signal_handler_disconnect(puss_get_doc_panel(self->app), self->switch_page_id);
	g_object_unref(self->builder);
	g_free(self);
}

