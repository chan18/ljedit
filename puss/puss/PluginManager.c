// PluginManager.c
//

#include "PluginManager.h"

#include <glib.h>
#include <gmodule.h>
#include <memory.h>
#include <string.h>

#include "Puss.h"
#include "Utils.h"
#include "DLLPluginEngine.h"

#define PLUGIN_SUFFIX ".plugin"

typedef struct _PluginEngine   PluginEngine;

struct _Plugin {
	gchar*			id;
	PluginEngine*	engine;
	gpointer		handle;

	Plugin*			next;
};

struct _PluginEngine {
	gchar*				key;
	PluginLoader		load;
	PluginUnloader		unload;
	PluginEngineDestroy	destroy;
	gpointer			tag;
};

static GtkDialog* g_plugin_manager_dlg = 0;
static GHashTable* puss_plugin_engines_map = 0;

static void unload_plugin( Plugin* plugin ) {
	if( plugin ) {
		if( plugin->handle && plugin->engine && plugin->engine->unload )
			plugin->engine->unload(plugin->handle, plugin->engine->tag);

		g_free(plugin->id);
	}
}

typedef gboolean TPluginWalkerFn(const gchar* plugin_id, const gchar* engine_id, GKeyFile* keyfile, gpointer tag);

static gboolean walk_read_plugin(const gchar* plugin_id, const gchar* filepath, TPluginWalkerFn cb, gpointer tag) {
	gchar* engine_id;
	GKeyFile* keyfile;
	GError* error;
	gboolean res = FALSE;

	keyfile = g_key_file_new();
	if( keyfile ) {
		error = 0;
		g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_NONE, &error);
		if( error ) {
			g_printerr( "load plugin error : load (%s).plugin failed(%s)!\n", plugin_id, error->message );
			g_error_free(error);

		} else {
			error = 0;
			engine_id = g_key_file_get_value(keyfile, "plugin", "engine", &error);
			if( error ) {
				g_printerr( "get engine-key error when load plugin[%s] - (%s)\n", plugin_id, error->message );
				g_error_free(error);
			}

			res = (*cb)(plugin_id, engine_id, keyfile, tag);

			g_free(engine_id);
		}
		g_key_file_free(keyfile);
	}

	return res;
}

static gboolean walk_one_plugin(const gchar* plugin_id, TPluginWalkerFn cb, gpointer tag) {
	gchar* filename;
	gchar* filepath;
	gboolean res = FALSE;

	filename = g_strconcat(plugin_id, PLUGIN_SUFFIX, NULL);
	if( filename ) {
		filepath = g_build_filename(puss_app->plugins_path, filename, NULL);
		if( filepath ) {
			res = walk_read_plugin(plugin_id, filepath, cb, tag);
			g_free(filepath);
		}
		g_free(filename);
	}

	return res;
}

static void walk_all_plugin(TPluginWalkerFn cb, gpointer tag) {
	const gchar* filename;
	gchar* plugin_path;
	GDir* dir;
	gsize len;
	gchar* plugin_id;

	if( !g_module_supported() )
		return;

	plugin_path = puss_app->plugins_path;
	if( !plugin_path )
		return;

	dir = g_dir_open(plugin_path, 0, NULL);
	if( dir ) {
		for(;;) {
			filename = g_dir_read_name(dir);
			if( !filename )
				break;

			len = (gsize)strlen(filename);
			if( !g_str_has_suffix(filename, PLUGIN_SUFFIX) )
				continue;

			plugin_id = g_strndup(filename, len - strlen(PLUGIN_SUFFIX));
			walk_one_plugin(plugin_id, cb, tag);
			g_free(plugin_id);
		}

		g_dir_close(dir);
	}
}

static gboolean do_load_plugin(const gchar* plugin_id, const gchar* engine_id, GKeyFile* keyfile, gpointer tag) {
	Plugin* plugin;

	plugin = g_try_new0(Plugin, 1);
	if( plugin ) {
		plugin->id = g_strdup(plugin_id);
		plugin->engine = (PluginEngine*)g_hash_table_lookup(puss_plugin_engines_map, engine_id?engine_id:DLL_PLUGIN_ENGINE_ID);;
		if( plugin->id && plugin->engine ) {
			plugin->handle = plugin->engine->load( plugin->id
				, keyfile
				, plugin->engine->tag );

			if( plugin->handle ) {
				plugin->next = puss_app->plugins_list;
				puss_app->plugins_list = plugin;
				return TRUE;
			}
		}

		unload_plugin(plugin);
		g_free(plugin);

	} else {
		g_printerr( "new Plugin memory error!\n" );
	}

	return FALSE;
}

gboolean puss_plugin_manager_load_plugin(const gchar* plugin_id) {
	return walk_one_plugin(plugin_id, do_load_plugin, 0);
}

void puss_plugin_manager_unload_plugin(const gchar* plugin_id) {
	Plugin* last;
	Plugin* p;

	last = 0;
	for( p = puss_app->plugins_list; p; p = p->next ) {
		if( g_str_equal(p->id, plugin_id) ) {
			if( last )
				last->next = p->next;
			else
				puss_app->plugins_list = p->next;

			unload_plugin(p);
			g_free(p);
			break;
		}
		last = p;
	}
}

void puss_plugin_manager_load_all() {
	walk_all_plugin(do_load_plugin, 0);
}

void puss_plugin_manager_unload_all() {
	Plugin* p;
	Plugin* t;

	p = puss_app->plugins_list;
	puss_app->plugins_list = 0;

	while( p ) {
		t = p;
		p = p->next;

		unload_plugin(t);
	}
}

static void puss_plugin_engine_destroy(PluginEngine* engine) {
	if( engine ) {
		if( engine->destroy )
			engine->destroy(engine->tag);
		g_free(engine->key);
		g_free(engine);
	}
}

gboolean puss_plugin_manager_create() {
	puss_plugin_engines_map = g_hash_table_new_full(g_str_hash, g_str_equal, 0, (GDestroyNotify)puss_plugin_engine_destroy);
	if( !puss_plugin_engines_map )
		return FALSE;

	dll_plugin_engine_regist(puss_app->api);

	return TRUE;
}

void puss_plugin_manager_destroy() {
	g_hash_table_destroy(puss_plugin_engines_map);
}

static gboolean do_fill_plugin_list(const gchar* plugin_id, const gchar* engine_id, GKeyFile* keyfile, gpointer tag) {
	GtkTreeIter iter;
	gboolean enabled = FALSE;
	GtkListStore* store = (GtkListStore*)tag;
	Plugin* pp = puss_app->plugins_list;

	for( ; pp; pp = pp->next ) {
		if( g_str_equal(pp->id, plugin_id) ) {
			enabled = TRUE;
			break;
		}
	}

	gtk_list_store_append(store, &iter);
	gtk_list_store_set( store, &iter
		, 0, enabled
		, 1, plugin_id
		, 2, engine_id
		, 3, g_key_file_get_string(keyfile, "plugin", "description", 0)
		, -1 );

	return FALSE;
}

static void plugin_ui_enabled_toggled(GtkCellRendererToggle* cell, gchar* path_str, gpointer data) {
	GtkListStore* store = (GtkListStore*)data;
	gboolean enabled;
	gchar* plugin_id;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &enabled, 1, &plugin_id, -1);

	enabled = !enabled;
	if( enabled ) {
		enabled = puss_plugin_manager_load_plugin(plugin_id);
	} else {
		puss_plugin_manager_unload_plugin(plugin_id);
	}

	gtk_list_store_set(store, &iter, 0, enabled, -1);
	gtk_tree_path_free (path);
}

SIGNAL_CALLBACK gboolean cb_puss_plugin_manager_dialog_delete(GtkWidget *widget) {
	gtk_widget_destroy(widget);
	g_plugin_manager_dlg = 0;
	return TRUE;
}

void puss_plugin_manager_show_config_dialog() {
	gchar* filepath;
	GtkBuilder* builder;
	GtkListStore* store;
	GtkCellRendererToggle* toggle;
	GError* err = 0;
	GtkWidget* dlg = (GtkWidget*)g_plugin_manager_dlg;

	if( !dlg ) {
		// create UI
		builder = gtk_builder_new();
		if( !builder )
			return;
		gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

		filepath = g_build_filename(puss_app->module_path, "res", "puss_plugin_manager_dialog.ui", NULL);
		if( !filepath ) {
			g_printerr("ERROR(puss) : build plugin manager ui filepath failed!\n");
			g_object_unref(G_OBJECT(builder));
			return;
		}

		gtk_builder_add_from_file(builder, filepath, &err);
		g_free(filepath);

		if( err ) {
			g_printerr("ERROR(puss): %s\n", err->message);
			g_error_free(err);
			g_object_unref(G_OBJECT(builder));
			return;
		}

		dlg = GTK_WIDGET(gtk_builder_get_object(builder, "puss_plugin_manager_dialog"));
		store = GTK_LIST_STORE(gtk_builder_get_object(builder, "plugin_store"));
		toggle = GTK_CELL_RENDERER_TOGGLE(gtk_builder_get_object(builder, "enabled_cell_renderer"));
		g_signal_connect(toggle, "toggled", G_CALLBACK(plugin_ui_enabled_toggled), store);

#if GTK_MAJOR_VERSION==2
		gtk_tree_view_column_set_sort_column_id(GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "enabled_column")), 0);
		gtk_tree_view_column_set_sort_column_id(GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "name_column")), 1);
		gtk_tree_view_column_set_sort_column_id(GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "engine_column")), 2);
#endif

		gtk_builder_connect_signals(builder, GTK_WINDOW(dlg));
		g_object_unref(G_OBJECT(builder));

		gtk_window_set_transient_for(GTK_WINDOW(dlg), puss_app->main_window);

		// fill data and show
		walk_all_plugin(do_fill_plugin_list, store);

		g_plugin_manager_dlg = (GtkDialog*)dlg;
	}

	gtk_widget_show_all(dlg);
	gdk_window_raise(gtk_widget_get_window(dlg));
}

void puss_plugin_engine_regist( const gchar* key
		, PluginLoader loader
		, PluginUnloader unloader
		, PluginEngineDestroy destroy
		, gpointer tag )
{
	PluginEngine* engine = g_try_new0(PluginEngine, 1);
	if( engine ) {
		engine->key = g_strdup(key);
		engine->load = loader;
		engine->unload = unloader;
		engine->destroy = destroy;
		engine->tag = tag;

		g_hash_table_insert(puss_plugin_engines_map, engine->key, engine); 
	}
}

