// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

#include "MainWindow.h"
#include "MiniLine.h"

struct Puss {
	MainWindow*	main_window;
	MiniLine*	mini_line;
};

void puss_create(Puss* app);
void puss_destroy(Puss* app);
void puss_run(Puss* app);

#endif//PUSS_INC_PUSS_H

