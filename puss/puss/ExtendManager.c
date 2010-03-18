// ExtendManager.c
//

#include "ExtendManager.h"

#include <glib.h>
#include <gmodule.h>
#include <memory.h>
#include <string.h>

#ifdef G_OS_WIN32

	#ifdef _DEBUG
		#define puss_g_module_open g_module_open
	#else
		#include <windows.h>

		GModule* puss_g_module_open(const gchar  *file_name, GModuleFlags  flags) {
			// do not show not find DLL dialog
			UINT uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
			GModule* ret = g_module_open(file_name, flags);
			SetErrorMode(uOldErrorMode);
			return ret;
		}
	#endif

#else
	#define puss_g_module_open g_module_open

#endif

#include "Puss.h"
#include "Utils.h"

typedef struct _Extend         Extend;

struct _Extend {
	gchar*		name;
	GModule*	module;
	void*		handle;
	Extend*		next;
};

static Extend* puss_extends_list = 0;

Extend* extend_load(const gchar* filepath) {
	void* (*create_fun)(Puss* app);
	Extend* extend = g_try_new0(Extend, 1);
	if( !extend )
		return 0;

	extend->module = puss_g_module_open(filepath, G_MODULE_BIND_LAZY);
	if( !extend->module ) {
		g_printerr(_("ERROR  : load extend(%s) failed!\n"), filepath);
		g_printerr(_("REASON : %s\n"), g_module_error());

	} else {
		g_module_symbol( extend->module
						, "puss_extend_create"
						, (gpointer*)&create_fun );

		if( !create_fun ) {
			g_printerr(_("ERROR  : not find puss_extend_create() in extend(%s)!\n"), filepath);

		} else {
			extend->handle = (*create_fun)(puss_app->api);
			return extend;
		}
	}

	if( extend->module )
		g_module_close(extend->module);

	g_free(extend);
	return 0;
}

void extend_init(Extend* extend) {
	void (*init_fun)(void* handle);
	if( extend && extend->module ) {
		g_module_symbol(extend->module, "puss_extend_init", (gpointer*)&init_fun);
		if( init_fun )
			(*init_fun)(extend->handle);
	}
}

void extend_final(Extend* extend) {
	void (*final_fun)(void* handle);
	if( extend && extend->module ) {
		g_module_symbol(extend->module, "puss_extend_final", (gpointer*)&final_fun);
		if( final_fun )
			(*final_fun)(extend->handle);
	}
}

void extend_unload(Extend* extend) {
	void (*destroy_fun)(void* handle);
	if( extend ) {
		if( extend->module ) {
			g_module_symbol(extend->module, "puss_extend_destroy", (gpointer*)&destroy_fun);

			if( destroy_fun )
				(*destroy_fun)(extend->handle);

			g_module_close(extend->module);
		}

		g_free(extend);
	}
}

gboolean puss_extend_manager_create() {
	gchar* extends_path;
	GDir* dir;
	Extend* extend;
	const gchar* filename;
	gsize len;
	gchar* filepath;

#ifdef _DEBUG
	const gchar* match_str = ".ext_d";
#else
	const gchar* match_str = ".ext";
#endif

	gsize match_len = (gsize)strlen(match_str);

	if( !g_module_supported() )
		return TRUE;

	extends_path = puss_app->extends_path;
	if( !extends_path )
		return TRUE;

	dir = g_dir_open(extends_path, 0, NULL);
	if( dir ) {
		for(;;) {
			filename = g_dir_read_name(dir);
			if( !filename )
				break;

			len = (gsize)strlen(filename);
			if( len < match_len || strcmp(filename+len-match_len, match_str)!=0 )
				continue;

			filepath = g_build_filename(extends_path, filename, NULL);
			extend = extend_load(filepath);
			g_free(filepath);

			if( extend ) {
				extend->name = g_strndup(filename, len - match_len);
				extend->next = puss_extends_list;
				puss_extends_list = extend;
				g_hash_table_insert(puss_app->extends_map, extend->name, extend);
			}
		}

		g_dir_close(dir);
	}

	extend = puss_extends_list;
	while( extend ) {
		extend_init(extend);
		extend = extend->next;
	}

	return TRUE;
}

void puss_extend_manager_destroy() {
	Extend* p;
	Extend* t;

	p = puss_extends_list;
	while( p ) {
		extend_final(p);
		p = p->next;
	}

	p = puss_extends_list;
	puss_extends_list = 0;

	while( p ) {
		t = p;
		p = p->next;

		extend_unload(t);
	}
}

gpointer puss_extend_manager_query(const gchar* ext_name, const gchar* interface_name) {
	void* (*query_fun)(void* handle, const gchar* interface_name);
	Extend* extend = g_hash_table_lookup(puss_app->extends_map, ext_name);
	if( extend ) {
		if( extend->module ) {
			g_module_symbol(extend->module, "puss_extend_query", (gpointer*)&query_fun);
			if( query_fun ) {
				return (*query_fun)(extend->handle, interface_name);
			}
		}
	}
	return 0;
}

