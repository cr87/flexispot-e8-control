#ifndef NTP_MAIN_H
#define NTP_MAIN_H

/*****************************************************************************/

#include "core.h"

/*****************************************************************************/

extern void ntp_init(const char *p_ntp_server, const int utc_offset_i32);
extern void ntp_loop();

extern void ntp_set_pool_name(const char *p_pool_name);
extern void ntp_set_time_offset(const int utc_offset_i32);

extern const DATETIME *ntp_get_current_time();

/*****************************************************************************/

#endif