// main.c

#include <libintl.h>
#include <gtk/gtk.h>

#include "Puss.h"

#define PACKAGE		"puss"
#define LOCALEDIR	"locale"

int main(int argc, char* argv[]) {
	gtk_set_locale();
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gtk_init(&argc, &argv);

	Puss* app = puss_create();
	if( !app )
		return 1;

	puss_run(app);
	puss_destroy(app);

	return 0;
}

