#ifndef CORE_H
#define CORE_H

/*****************************************************************************/

#include <c_types.h>
#include <Arduino.h>

/*****************************************************************************/

#define UNSIGNED_DIFF(a, b) ((a) >= (b) ? ((a) - (b)) : ((b) - (a)))
#define ONCE_EVERY_MS(ms)                     \
    {                                         \
        static unsigned long last = millis(); \
        if ((millis() - last) < (ms))         \
        {                                     \
            return;                           \
        }                                     \
        else                                  \
        {                                     \
            last = millis();                  \
        }                                     \
    }

/*****************************************************************************/

typedef enum
{
    DC_CMD_INVALID = 0,
    DC_CMD_WAKEUP,
    DC_CMD_UP,
    DC_CMD_DOWN,
    DC_CMD_M,
    DC_CMD_PRESET_1,
    DC_CMD_PRESET_2,
    DC_CMD_PRESET_3,
    DC_CMD_PRESET_4
} DC_COMMAND;

typedef enum
{
    DC_STATE_UNKNOWN = 0,
    DC_STATE_STANDING,
    DC_STATE_SITTING
} DC_STATE;

/*****************************************************************************/

typedef enum
{
    SUNDAY = 0,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    NUM_WEEKDAYS
} WEEKDAY;

typedef struct
{
    uint8 hour_u8;
    uint8 minute_u8;
    uint8 second_u8;
} TIME;

typedef struct
{
    uint16 year_u16;
    uint8 month_u8;
    uint8 day_u8;
    WEEKDAY day_of_week_e;
} DATE;

typedef struct
{
    TIME time;
    DATE date;
} DATETIME;

/*****************************************************************************/

/* Defines the schedule for a day */
typedef struct
{
    TIME start_time;
    TIME end_time;
    uint16 interval_u16;
    uint16 duration_u16;
    int enabled;
} DAY_CONFIG;

/*****************************************************************************/

/* Provider function pointers */
typedef uint16 (*fn_height_provider)(void);
typedef const DATETIME *(*fn_time_provider)(void);
typedef DC_STATE (*fn_desk_state_provider)(void);

/* Receiver function pointers */
typedef int (*fn_command_receiver)(const DC_COMMAND);
typedef void (*fn_schedule_receiver)(const WEEKDAY day_e, const DAY_CONFIG *p_config);

/*****************************************************************************/

extern sint32 time_diff(const TIME *p_a, const TIME *p_b);                              /* returns 1: a > b, -1: a < b, 0: a == b */
extern uint32 time_add(const TIME *p_time, TIME *p_time_out, const uint32 seconds_u32); /* returns number of days carried over */
extern sint32 time_to_seconds(const TIME *p_time);

/*****************************************************************************/

#endif