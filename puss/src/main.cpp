// main.c

#include <libintl.h>

#include "Puss.h"

#define PACKAGE		"puss"
#define LOCALEDIR	"locale"

int main(int argc, char* argv[]) {
	gtk_set_locale();
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gtk_init(&argc, &argv);

	Puss puss;
	puss_create(&puss);
	puss_run(&puss);

	return 0;
}

