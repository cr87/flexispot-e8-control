#ifndef WS_MAIN_H
#define WS_MAIN_H

/*****************************************************************************/

#include "core.h"

/*****************************************************************************/

extern void ws_init(const uint16 server_port_u16,
                    fn_height_provider height_provider,
                    fn_command_receiver command_receiver,
                    fn_time_provider time_provider);
extern void ws_loop();

/*****************************************************************************/

#endif