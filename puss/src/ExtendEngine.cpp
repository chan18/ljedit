// ExtendEngine.cpp
//

#include "ExtendEngine.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <memory.h>
#include <string.h>

#include "IPuss.h"

struct Extend {
	GModule*	module;
	void*		handle;
	Extend*		next;
};

Extend* extend_load(Puss* app, const gchar* filepath) {
	Extend* extend = (Extend*)g_malloc(sizeof(Extend));
	memset(extend, 0, sizeof(Extend));

	extend->module = g_module_open(filepath, G_MODULE_BIND_LAZY);
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
	GDir* dir = g_dir_open(extends_dir, 0, NULL);
	if( !dir )
		return;

	Extend* extend = 0;
	for(;;) { 
		const gchar* filename = g_dir_read_name(dir);
		if( !filename )
			break;

		size_t len = strlen(filename);
		if(    len < 5
			|| filename[len-4] != '.'
			|| filename[len-3] != 'e'
			|| filename[len-2] != 'x'
			|| filename[len-1] != 't' )
		{
			continue;
		}

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

void puss_extend_engine_destroy(Puss* app) {
	while( extends_list ) {
		Extend* p = extends_list->next;
		extend_unload(extends_list);
		extends_list = p;
	}
}

