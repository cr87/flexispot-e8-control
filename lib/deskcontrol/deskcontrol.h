#ifndef DC_MAIN_H
#define DC_MAIN_H

/*****************************************************************************/

#include "core.h"

/*****************************************************************************/

extern void dc_init();
extern void dc_loop();

extern void dc_set_params(uint16 height_standing_u16, uint16 height_sitting_u16, uint16 height_tolerance_u16);

extern int dc_send_cmd(const DC_COMMAND cmd_e);
extern void dc_activate();

extern uint16 dc_get_current_height();
extern DC_STATE dc_get_current_state();

/*****************************************************************************/

#endif