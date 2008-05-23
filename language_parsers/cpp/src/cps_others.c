// cps_others.c
// 

#include "cps_utils.h"


gboolean cps_extern_scope(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_block(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_impl_block(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_label(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_fun_or_var(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_breakout_block(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_skip_block(Block* block, GList* scope) {
	return TRUE;
}

gboolean cps_class_or_var(Block* block, GList* scope) {
	return TRUE;
}

