// Puss.h
//

#ifndef PUSS_INC_PUSS_H
#define PUSS_INC_PUSS_H

struct Puss;

Puss*	puss_create();
void	puss_destroy(Puss* app);

void	puss_run(Puss* app);

#endif//PUSS_INC_PUSS_H

