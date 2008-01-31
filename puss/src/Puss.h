// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "UI.h"

struct Puss {
	UI ui;
};

void puss_create(Puss* app);
void puss_run(Puss* app);

#endif//PUSS_INC_PUSS_H

