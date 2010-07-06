// vdriver.h
// 
#ifndef PUSS_MI_VDRIVER_H
#define PUSS_MI_VDRIVER_H

#include "protocol.h"

typedef struct _MIVDriver MIVDriver;

typedef enum { MI_TARGET_ST_NONE
	, MI_TARGET_ST_DONE
	, MI_TARGET_ST_RUNNING
	, MI_TARGET_ST_STOPPED
	, MI_TARGET_ST_CONNECTED
	, MI_TARGET_ST_ERROR
	, MI_TARGET_ST_EXIT
} MITargetStatus;

typedef struct {
	gchar*		gdb;
	gchar*		exec;
	gchar*		args;
	gchar*		working_dir;
	gchar**		env;
	gboolean	succeed;
} MITargetSetup;

MITargetSetup*	mi_target_setup_new();
void			mi_target_setup_clear(MITargetSetup* setup);
void			mi_target_setup_free(MITargetSetup* setup);

typedef void (*MICommandMonitor)(MIVDriver* drv, const gchar* cmd, gpointer tag);
typedef void (*MIMessageMonitor)(MIVDriver* drv, MIRecord* record, const gchar* msg, gpointer tag);
typedef void (*MITargetStatusChanged)(MIVDriver* drv, MITargetStatus status, MIRecord* record, gpointer tag);

MIVDriver*		mi_vdriver_new();
void			mi_vdriver_free(MIVDriver* drv);

MITargetStatus	mi_vdriver_get_target_status(MIVDriver* self);

void			mi_vdriver_set_callbacks( MIVDriver* self
						, MICommandMonitor cb_command_monitor
						, MIMessageMonitor cb_message_monitor
						, MITargetStatusChanged cb_target_status_changed
						, gpointer tag
						, GFreeFunc tag_free );

void			mi_vdriver_set_target_setup(MIVDriver* self, MITargetSetup* setup);
MITargetSetup*	mi_vdriver_get_target_setup(MIVDriver* self);

gboolean mi_vdriver_command_run(MIVDriver* self);
gboolean mi_vdriver_command_pause(MIVDriver* self);
gboolean mi_vdriver_command_stop(MIVDriver* self);

gboolean mi_vdirver_command_send(MIVDriver* self, gchar* cmd, ...);

#endif//PUSS_MI_VDRIVER_H

