// ExtendEngine.cpp
//

#include "ExtendEngine.h"

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

typedef struct _Extend {
	GModule*	module;
	void*		handle;
	Extend*		next;
} Extend;

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
			extend->handle = (*create_fun)((Puss*)puss_app);
			return extend;
		}
	}

	if( extend->module )
		g_module_close(extend->module);

	g_free(extend);
	return 0;
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

gboolean puss_extend_engine_create() {
	gchar* extends_dir;
	GDir* dir;
	Extend* extend;
	const gchar* filename;
	size_t len;
	gchar* filepath;
	const gchar* match_str = ".ext";
	size_t match_len = strlen(match_str);


	if( !g_module_supported() )
		return TRUE;

	extends_dir = g_build_filename(puss_app->module_path, "extends", NULL);
	if( !extends_dir )
		return TRUE;

	dir = g_dir_open(extends_dir, 0, NULL);
	if( dir ) {
		for(;;) { 
			filename = g_dir_read_name(dir);
			if( !filename )
				break;

			len = strlen(filename);
			if( len < match_len || strcmp(filename+len-match_len, match_str)!=0 )
				continue;

			filepath = g_build_filename(extends_dir, filename, NULL);
			extend = extend_load(filepath);
			g_free(filepath);

			if( extend ) {
				extend->next = puss_app->extends_list;
				puss_app->extends_list = extend;
			}
		}

		g_dir_close(dir);
	}

	g_free(extends_dir);

	return TRUE;
}

void puss_extend_engine_destroy() {
	Extend* t;
	Extend* p = puss_app->extends_list;
	puss_app->extends_list = 0;

	while( p ) {
		t = p;
		p = p->next;

		extend_unload(t);
	}

}

