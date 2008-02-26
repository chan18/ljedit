// ExtendEngine.cpp
//

#include "ExtendEngine.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <memory.h>
#include <string.h>

#ifdef WIN32
	#include <windows.h>

	GModule* puss_g_module_open(const gchar  *file_name, GModuleFlags  flags) {
		// do not show not find DLL dialog
		UINT uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
		GModule* ret = g_module_open(file_name, flags);
		SetErrorMode(uOldErrorMode);
		return ret;
	}

#else
	#define puss_g_module_open g_module_open

#endif

#include "IPuss.h"

struct Extend {
	GModule*	module;
	void*		handle;
	Extend*		next;
};

Extend* extend_load(Puss* app, const gchar* filepath) {
	Extend* extend = (Extend*)g_malloc(sizeof(Extend));
	memset(extend, 0, sizeof(Extend));

	extend->module = puss_g_module_open(filepath, G_MODULE_BIND_LAZY);
	if( !extend->module ) {
		g_printerr(_("ERROR  : load extend(%s) failed!\n"), filepath);
		g_printerr(_("REASON : %s\n"), g_module_error());

	} else {
		void* (*create_fun)(Puss* app);
		g_module_symbol( extend->module
						, "puss_extend_create"
						, (gpointer*)&create_fun );

		if( !create_fun ) {
			g_printerr(_("ERROR  : not find puss_extend_create() in extend(%s)!\n"), filepath);

		} else {
			extend->handle = (*create_fun)(app);
			return extend;
		}
	}

	if( extend->module )
		g_module_close(extend->module);

	g_free(extend);
	return 0;
}

void extend_unload(Extend* extend) {
	if( extend ) {
		if( extend->module ) {
			void (*destroy_fun)(void* handle);
			g_module_symbol(extend->module, "puss_extend_destroy", (gpointer*)&destroy_fun);

			if( destroy_fun )
				(*destroy_fun)(extend->handle);

			g_module_close(extend->module);
		}

		g_free(extend);
	}
}

Extend* extends_list = 0;

void puss_extend_engine_create(Puss* app) {
	if( !g_module_supported() )
		return;

	gchar* extends_dir = g_build_filename(app->module_path, "extends", NULL);
	if( !extends_dir )
		return;

	GDir* dir = g_dir_open(extends_dir, 0, NULL);
	if( dir ) {
		Extend* extend = 0;

		const gchar* match_str = ".ext";
		size_t match_len = strlen(match_str);

		for(;;) { 
			const gchar* filename = g_dir_read_name(dir);
			if( !filename )
				break;

			size_t len = strlen(filename);
			if( len < match_len || strcmp(filename+len-match_len, match_str)!=0 )
				continue;

			gchar* filepath = g_build_filename(extends_dir, filename, NULL);
			extend = extend_load(app, filepath);
			g_free(filepath);

			if( extend ) {
				extend->next = extends_list;
				extends_list = extend;
			}
		}

		g_dir_close(dir);
	}

	g_free(extends_dir);
}

void puss_extend_engine_destroy(Puss* app) {
	while( extends_list ) {
		Extend* p = extends_list->next;
		extend_unload(extends_list);
		extends_list = p;
	}
}

