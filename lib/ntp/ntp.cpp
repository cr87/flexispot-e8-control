#include "ntp.h"

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#include "log.h"

/*****************************************************************************/

WiFiUDP ntp_udp;
NTPClient ntp_client(ntp_udp);
DATETIME ntp_current_time;

/*****************************************************************************/

void ntp_reset_time();

/*****************************************************************************/

void ntp_init(const char *p_ntp_server, const int utc_offset_i32)
{
    ntp_reset_time();

    ntp_set_pool_name(p_ntp_server);
    ntp_set_time_offset(utc_offset_i32);

    ntp_client.begin();
}

void ntp_loop()
{
    time_t t;
    tm local_time;

    ntp_client.update();

    t = ntp_client.getEpochTime();
    localtime_r(&t, &local_time);

    /* Convert to our format */
    ntp_current_time.date.year_u16 = local_time.tm_year + 1900U;
    ntp_current_time.date.month_u8 = local_time.tm_mon + 1U;
    ntp_current_time.date.day_u8 = local_time.tm_mday;
    ntp_current_time.date.day_of_week_e = (WEEKDAY)min((int)NUM_WEEKDAYS, local_time.tm_wday);
    ntp_current_time.time.hour_u8 = local_time.tm_hour;
    ntp_current_time.time.minute_u8 = local_time.tm_min;
    ntp_current_time.time.second_u8 = local_time.tm_sec;
}

void ntp_set_pool_name(const char *p_pool_name)
{
    log_msg(LOG_LEVEL_INFO, "NTP", "Setting pool name to '%s'.", p_pool_name);
    ntp_client.setPoolServerName(p_pool_name);
}

void ntp_set_time_offset(const int utc_offset_i32)
{
    log_msg(LOG_LEVEL_INFO, "NTP", "Setting time offset to %i seconds.", utc_offset_i32);
    ntp_client.setTimeOffset(utc_offset_i32);
}

const DATETIME *ntp_get_current_time()
{
    return &ntp_current_time; /* only safe because we have no concurrency */
}

void ntp_reset_time()
{
    (void)memset(&ntp_current_time, 0, sizeof(DATETIME));
}