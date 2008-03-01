// PosLocate.h
// 

#ifndef PUSS_INC_POSLOCATE_H
#define PUSS_INC_POSLOCATE_H

#include <gtk/gtk.h>

struct Puss;

void	puss_pos_locate_add( Puss* app, gint page_num, gint line, gint offset);

void	puss_pos_locate_forward( Puss* app );
void	puss_pos_locate_back( Puss* app );


#endif//PUSS_INC_POSLOCATE_H

