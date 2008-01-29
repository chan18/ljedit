// main.c

#include "Puss.h"

#define PACKAGE		"puss"
#define LOCALEDIR	"locale"

int main(int argc, char* argv[]) {
	gtk_set_locale();
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	gtk_init(&argc, &argv);

	Puss puss;
	puss.create();

	puss.run();
}

