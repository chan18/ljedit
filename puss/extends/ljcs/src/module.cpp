// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>
#include <gmodule.h>

#include "LJCS.h"

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	LJCS* self = new LJCS;
	if( self ) {
		if( !self->create(app) ) {
			delete self;
			self = 0;
		}
	}
	return self;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	delete (LJCS*)self;
}

