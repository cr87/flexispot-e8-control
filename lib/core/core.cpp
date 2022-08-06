#include "core.h"

#include <SoftwareSerial.h>

/*****************************************************************************/

sint32 time_diff(const TIME *p_a, const TIME *p_b)
{
    return time_to_seconds(p_a) - time_to_seconds(p_b);
}

uint32 time_add(const TIME *p_time, const uint32 seconds_u32, TIME *p_time_out)
{
    uint16 day_carryover_u16 = 0U;

    p_time_out->second_u8 = p_time->second_u8 + (seconds_u32 % 60U);
    p_time_out->minute_u8 = p_time->minute_u8 + ((seconds_u32 / 60U) % 60U);
    p_time_out->hour_u8 = p_time->hour_u8 + (seconds_u32 / 3600U);

    if (p_time_out->second_u8 >= 60U)
    {
        p_time_out->second_u8 -= 60U;
        p_time_out->minute_u8 += 1U;
    }
    else
    {
    }

    if (p_time_out->minute_u8 >= 60U)
    {
        p_time_out->minute_u8 -= 60U;
        p_time_out->hour_u8 += 1U;
    }
    else
    {
    }

    if (p_time_out->hour_u8 >= 24U)
    {
        day_carryover_u16 = (p_time_out->hour_u8 / 24U);
        p_time_out->hour_u8 = (p_time_out->hour_u8 % 24U);
    }
    else
    {
    }

    return day_carryover_u16;
}

/*****************************************************************************/

sint32 time_to_seconds(const TIME *p_time)
{
    return ((sint32)p_time->hour_u8 * 3600) + ((sint32)p_time->minute_u8 * 60) + (sint32)p_time->second_u8;
}

/*****************************************************************************/
