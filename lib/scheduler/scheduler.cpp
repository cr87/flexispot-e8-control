#include "scheduler.h"

#include <string.h>

#include "log.h"

/*****************************************************************************/

typedef enum
{
    SC_STATE_INACTIVE = 0,
    SC_STATE_TRANSITION_SIT_TO_STAND,
    SC_STATE_TRANSITION_STAND_TO_SIT,
    SC_STATE_SITTING,
    SC_STATE_STANDING
} SCHEDULER_STATE;

/*****************************************************************************/

fn_desk_state_provider sc_desk_state_provider = NULL;
fn_command_receiver sc_desk_command_receiver = NULL;
fn_time_provider sc_time_provider = NULL;

DAY_CONFIG sc_day_configs[NUM_WEEKDAYS] = {0};
uint16 sc_config_transition_time_tolerance_u16 = 1U * 60U; /* 1 minute */
uint16 sc_config_command_send_time_tolerance_u16 = 30U;    /* 30 seconds */

DC_COMMAND sc_command_last_sent_e = DC_CMD_INVALID;
TIME sc_command_last_sent_time = {0};

/*****************************************************************************/

void sc_reset();
SCHEDULER_STATE sc_determine_state(const TIME *p_time, const DAY_CONFIG *p_config, const uint16 transition_time_tolerance_u16);
void sc_handle_target_state(const TIME *p_time, const SCHEDULER_STATE target_state_e);
void sc_handle_command_request(const TIME *p_time, const DC_COMMAND requested_command_e);

/*****************************************************************************/

void sc_init()
{
    sc_desk_state_provider = NULL;
    sc_desk_command_receiver = NULL;
    sc_time_provider = NULL;

    sc_reset();
}

void sc_loop()
{
    const DAY_CONFIG *p_config;
    const DATETIME *p_time;
    SCHEDULER_STATE target_state_e;

    /* Check for an active schedule and a potentially necessary state change - enough to do it every second */
    ONCE_EVERY_MS(1000U);
    if (NULL != sc_time_provider)
    {
        p_time = sc_time_provider();

        /* Determine the config of this weekday and then check whether the schedule is active */
        if (p_time->date.day_of_week_e < NUM_WEEKDAYS)
        {
            log_msg(LOG_LEVEL_DEBUG, "Scheduler", "Got time %i:%i:%i and date %i/%i%/%i (day of week %i)",
                    (int)p_time->time.hour_u8, (int)p_time->time.minute_u8, (int)p_time->time.second_u8,
                    (int)p_time->date.year_u16, (int)p_time->date.month_u8, (int)p_time->date.day_u8,
                    (int)p_time->date.day_of_week_e);

            p_config = &(sc_day_configs[p_time->date.day_of_week_e]);
            log_msg(LOG_LEVEL_INFO, "Scheduler", "Got config for day %i from %i:%i to %i:%i with enabled: %i",
                    (int)p_time->date.day_of_week_e,
                    p_config->start_time.hour_u8, p_config->start_time.minute_u8,
                    p_config->end_time.hour_u8, p_config->end_time.minute_u8,
                    p_config->enabled);

            target_state_e = sc_determine_state(&(p_time->time), p_config, sc_config_transition_time_tolerance_u16);
            sc_handle_target_state(&(p_time->time), target_state_e);
        }
        else
        {
            /* Something went weirdly wrong */
        }
    }
    else
    {
        log_msg(LOG_LEVEL_ERROR, "Scheduler", "No time provider set.");
    }
}

void sc_set_desk_state_provider(fn_desk_state_provider p_desk_state_provider)
{
    sc_desk_state_provider = p_desk_state_provider;
}

void sc_set_desk_command_receiver(fn_command_receiver p_desk_command_receiver)
{
    sc_desk_command_receiver = p_desk_command_receiver;
}

void sc_set_time_provider(fn_time_provider p_time_provider)
{
    sc_time_provider = p_time_provider;
}

void sc_set_day_configs(const DAY_CONFIG configs[NUM_WEEKDAYS])
{
    (void)memcpy(sc_day_configs, configs, sizeof(DAY_CONFIG) * NUM_WEEKDAYS);
}

void sc_set_day_config(const WEEKDAY day_e, const DAY_CONFIG *p_config)
{
    if ((day_e < NUM_WEEKDAYS) && (NULL != p_config))
    {
        log_msg(LOG_LEVEL_INFO, "Scheduler", "Setting day config for day %i", (int)day_e);
        (void)memcpy(&sc_day_configs[day_e], p_config, sizeof(DAY_CONFIG));
    }
    else
    {
        /* Yikes... */
    }
}

/*****************************************************************************/

void sc_reset()
{
    (void)memset(sc_day_configs, 0, sizeof(DAY_CONFIG) * NUM_WEEKDAYS);
    sc_command_last_sent_e = DC_CMD_INVALID;
    sc_command_last_sent_time = {0};
}

SCHEDULER_STATE sc_determine_state(const TIME *p_time, const DAY_CONFIG *p_config, const uint16 transition_time_tolerance_u16)
{
    SCHEDULER_STATE state_e = SC_STATE_INACTIVE;

    sint32 diff_to_start_s32;
    sint32 diff_to_end_s32;
    uint32 relative_time_u32;

    if (p_config->enabled > 0)
    {
        /* check whether time is within start and end time */
        diff_to_start_s32 = time_diff(p_time, &(p_config->start_time));
        diff_to_end_s32 = time_diff(p_time, &(p_config->end_time));

        if ((diff_to_start_s32 >= 0) && (diff_to_end_s32 <= 0) && (p_config->interval_u16 > 0U))
        {
            /* Compute relative time within interval to determine the state */
            relative_time_u32 = (uint16)diff_to_start_s32 % p_config->interval_u16;

            if (relative_time_u32 < transition_time_tolerance_u16)
            {
                state_e = SC_STATE_TRANSITION_SIT_TO_STAND;
            }
            else if (relative_time_u32 < p_config->duration_u16)
            {
                state_e = SC_STATE_STANDING;
            }
            else if (relative_time_u32 < (p_config->duration_u16 + transition_time_tolerance_u16))
            {
                state_e = SC_STATE_TRANSITION_STAND_TO_SIT;
            }
            else
            {
                state_e = SC_STATE_SITTING;
            }
        }
        else
        {
            /* State already inactive */
            log_msg(LOG_LEVEL_DEBUG, "Scheduler", "Outside of schedule (current time %u:%u:%u, start time %u:%u:%u, end time %u:%u:%u).",
                    p_time->hour_u8, p_time->minute_u8, p_time->second_u8,
                    p_config->start_time.hour_u8, p_config->start_time.minute_u8, p_config->start_time.second_u8,
                    p_config->end_time.hour_u8, p_config->end_time.minute_u8, p_config->end_time.second_u8);
        }
    }
    else
    {
        /* State already inactive */
        log_msg(LOG_LEVEL_INFO, "Scheduler", "Current schedule config is disabled");
    }

    return state_e;
}

void sc_handle_target_state(const TIME *p_time, const SCHEDULER_STATE target_state_e)
{
    DC_STATE desk_state_e;
    DC_COMMAND command_to_send_e = DC_CMD_INVALID;

    if (NULL != sc_desk_state_provider)
    {
        if (target_state_e != SC_STATE_INACTIVE)
        {
            /* Figure out from desk control which state the desk is in, and which we are in */
            desk_state_e = sc_desk_state_provider();
            log_msg(LOG_LEVEL_INFO, "Scheduler", "Target state is %i and desk state is %i.", target_state_e, desk_state_e);

            /* Now match our target state to the desk state */
            if ((desk_state_e == DC_STATE_SITTING) && ((target_state_e == SC_STATE_SITTING) || (target_state_e == SC_STATE_TRANSITION_STAND_TO_SIT)))
            {
                /* Already in requested sitting state */
            }
            else if ((desk_state_e == DC_STATE_STANDING) && ((target_state_e == SC_STATE_STANDING) || (target_state_e == SC_STATE_TRANSITION_SIT_TO_STAND)))
            {
                /* Already in requested standing state) */
            }
            else if ((desk_state_e == DC_STATE_SITTING) && (target_state_e == SC_STATE_TRANSITION_SIT_TO_STAND))
            {
                command_to_send_e = DC_CMD_PRESET_3;
            }
            else if ((desk_state_e == DC_STATE_STANDING) && (target_state_e == SC_STATE_TRANSITION_STAND_TO_SIT))
            {
                command_to_send_e = DC_CMD_PRESET_4;
            }
            else
            {
                /* Command already invalid */
            }

            sc_handle_command_request(p_time, command_to_send_e);
        }
        else
        {
            log_msg(LOG_LEVEL_INFO, "Scheduler", "State is inactive");
        }
    }
    else
    {
        log_msg(LOG_LEVEL_ERROR, "Scheduler", "No desk state provider set");
    }
}

void sc_handle_command_request(const TIME *p_time, const DC_COMMAND requested_command_e)
{
    sint32 diff_s32;

    /* Check whether we have a valid command that we are allowed to send (again) */
    if (requested_command_e != DC_CMD_INVALID)
    {
        /* Compute how long ago last command was sent */
        diff_s32 = time_diff(p_time, &sc_command_last_sent_time);
        if ((requested_command_e != sc_command_last_sent_e) || ((diff_s32 >= 0) && ((uint16)diff_s32 >= sc_config_command_send_time_tolerance_u16)))
        {
            /* Send the command. TODO handle failure case? */
            (void)sc_desk_command_receiver(requested_command_e);

            /* Store for next time */
            sc_command_last_sent_e = requested_command_e;
            sc_command_last_sent_time = *p_time;
        }
        else
        {
            /* We are not yet allowed to send the same command again */
            log_msg(LOG_LEVEL_INFO, "Scheduler", "Want to send command %i but time diff %i to last send time too small.", requested_command_e, diff_s32);
        }
    }
}

/*****************************************************************************/
