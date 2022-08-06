#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "core.h"

/*****************************************************************************/

extern void sc_init();
extern void sc_loop();

extern void sc_set_desk_state_provider(fn_desk_state_provider p_desk_state_provider);
extern void sc_set_desk_command_receiver(fn_command_receiver p_command_receiver);
extern void sc_set_time_provider(fn_time_provider p_time_provider);

extern void sc_set_day_configs(const DAY_CONFIG configs[NUM_WEEKDAYS]);
extern void sc_set_day_config(const WEEKDAY day_e, const DAY_CONFIG *p_config);

/*****************************************************************************/

#endif