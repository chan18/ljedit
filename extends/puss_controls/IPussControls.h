// IPussControls.h
//

#ifndef PUSS_INC_IPUSSCONTROLS_H
#define PUSS_INC_IPUSSCONTROLS_H

#include "IPuss.h"

#define INTERFACE_TOOL_MENU_ACTION	"ToolMenuAction"

typedef struct {
	GType		(*get_type)();
} IToolMenuAction;

#endif//PUSS_INC_IPUSSCONTROLS_H

