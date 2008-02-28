// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "IPuss.h"

struct PussImpl {
	Puss		parent;

	GHashTable*	suffix_map;
};

Puss*	puss_create(const char* filepath);
void	puss_destroy(Puss* app);

void	puss_run(Puss* app);

#endif//PUSS_INC_PUSS_H

