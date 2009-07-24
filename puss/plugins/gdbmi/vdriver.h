// vdriver.h
// 
#ifndef PUSS_MI_VDRIVER_H
#define PUSS_MI_VDRIVER_H

#include <glib.h>

typedef struct _MIVDriver MIVDriver;

MIVDriver*	mi_vdriver_new();
void		mi_vdriver_free();

gboolean mi_vdriver_command_start(MIVDriver* self);

#endif//PUSS_MI_VDRIVER_H

