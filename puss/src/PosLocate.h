// PosLocate.h
// 

#ifndef PUSS_INC_POSLOCATE_H
#define PUSS_INC_POSLOCATE_H

#include <gtk/gtk.h>

gboolean	puss_pos_locate_create();
void		puss_pos_locate_destroy();

void		puss_pos_locate_add(int page_num, int line, int offset);
void		puss_pos_locate_add_current_pos();

void		puss_pos_locate_current();
void		puss_pos_locate_forward();
void		puss_pos_locate_back();

#endif//PUSS_INC_POSLOCATE_H

