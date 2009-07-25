// vdriver.h
// 
#ifndef PUSS_MI_VDRIVER_H
#define PUSS_MI_VDRIVER_H

#include <glib.h>

typedef struct _MIVDriver MIVDriver;

typedef enum { MI_TARGET_ST_NONE
	, MI_TARGET_ST_DONE
	, MI_TARGET_ST_RUNNING
	, MI_TARGET_ST_STOPPED
	, MI_TARGET_ST_CONNECTED
	, MI_TARGET_ST_ERROR
	, MI_TARGET_ST_EXIT
} MITargetStatus;

MIVDriver*		mi_vdriver_new();
void			mi_vdriver_free(MIVDriver* drv);

MITargetStatus	mi_vdriver_target_status(MIVDriver* self);

gboolean mi_vdriver_command_start(MIVDriver* self);
gboolean mi_vdriver_command_run(MIVDriver* self);
gboolean mi_vdriver_command_pause(MIVDriver* self);
gboolean mi_vdriver_command_stop(MIVDriver* self);

gboolean mi_vdriver_command_continue(MIVDriver* self);

#endif//PUSS_MI_VDRIVER_H

