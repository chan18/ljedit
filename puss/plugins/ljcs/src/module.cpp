// module.cpp
//

#include "IPuss.h"

#include <gmodule.h>

#include "LJCS.h"

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	LJCS* self = new LJCS;
	if( self ) {
		if( !self->create(app) ) {
			delete self;
			self = 0;
		}
	}
	return self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* self) {
	delete (LJCS*)self;
}

