#include "deskcontrol.h"

#include <SoftwareSerial.h>

#include "log.h"

/*****************************************************************************/

#define DC_RX_BUFFER_SIZE 7U /* because we care about the height message */

/*****************************************************************************/

const uint32_t DC_SERIAL_BAUDRATE_U32 = 9600;
const int8_t DC_SERIAL_RX_PIN_I8 = 13; // D7 GPIO13
const int8_t DC_SERIAL_TX_PIN_I8 = 15; // D8 GPIO15
const uint8_t DC_COMMS_PIN20_U8 = 05;  // D1 GPI05
const char *dc_module_str = "Desk";

/*****************************************************************************/

SoftwareSerial dc_serial;
byte dc_rx_buffer_vu8[DC_RX_BUFFER_SIZE] = {0};
uint8 dc_rx_buffer_head_u8 = 0U;

uint16 dc_height_standing_u16 = 0U;
uint16 dc_height_sitting_u16 = 0U;
uint16 dc_height_tolerance_u16 = 0U;

DC_STATE dc_state_current_e = DC_STATE_UNKNOWN;
uint16 dc_state_current_height_u16 = 0U;
bool dc_state_currently_active_b = false;

TIME dc_height_query_last_command_time = {0};

/*****************************************************************************/

void dc_reset_read_buffer();
void dc_reset_current_state();

void dc_handle_serial();
void dc_handle_state();

void dc_parse_received_message(const byte *p_buffer, const uint8 size_u8);
uint16 dc_parse_height_from_buffer(const byte *p_buffer, const uint8 size_u8);
uint8 dc_parse_digit_from_byte(byte bt);

void dc_request_activation();

/*****************************************************************************/

void dc_init()
{
    /* Setup PIN20 which is used to "activate" the motor controller */
    pinMode(DC_COMMS_PIN20_U8, OUTPUT);
    digitalWrite(DC_COMMS_PIN20_U8, LOW);

    /* Set serial communication pins to appropriate modes */
    pinMode(DC_SERIAL_RX_PIN_I8, INPUT);
    pinMode(DC_SERIAL_TX_PIN_I8, OUTPUT);

    /* Start communication in 8N1 mode */
    dc_serial.begin(DC_SERIAL_BAUDRATE_U32, SWSERIAL_8N1, DC_SERIAL_RX_PIN_I8, DC_SERIAL_TX_PIN_I8);

    /* Prepare buffer */
    dc_reset_read_buffer();

    /* Reset state */
    dc_set_params(0U, 0U, 0U);
    dc_reset_current_state();
}

void dc_loop()
{
    /* Make sure we handle all incoming data */
    dc_handle_serial();

    /* Keep state up to date */
    dc_handle_state();
}

void dc_set_params(uint16 height_standing_u16, uint16 height_sitting_u16, uint16 height_tolerance_u16)
{
    dc_height_standing_u16 = height_standing_u16;
    dc_height_sitting_u16 = height_sitting_u16;
    dc_height_tolerance_u16 = height_tolerance_u16;
}

/*****************************************************************************/

int dc_send_cmd(const DC_COMMAND cmd_e)
{
    int ret = 0;

    const byte CMD_WAKEUP[] = {0x9b, 0x06, 0x02, 0x00, 0x00, 0x6c, 0xa1, 0x9d};
    const byte CMD_UP[] = {0x9b, 0x06, 0x02, 0x01, 0x00, 0xfc, 0xa0, 0x9d};
    const byte CMD_DOWN[] = {0x9b, 0x06, 0x02, 0x02, 0x00, 0x0c, 0xa0, 0x9d};
    const byte CMD_M[] = {0x9b, 0x06, 0x02, 0x20, 0x00, 0xac, 0xb8, 0x9d};
    const byte CMD_PRESET_1[] = {0x9b, 0x06, 0x02, 0x04, 0x00, 0xac, 0xa3, 0x9d};
    const byte CMD_PRESET_2[] = {0x9b, 0x06, 0x02, 0x08, 0x00, 0xac, 0xa6, 0x9d};
    const byte CMD_PRESET_3[] = {0x9b, 0x06, 0x02, 0x10, 0x00, 0xac, 0xac, 0x9d};
    const byte CMD_PRESET_4[] = {0x9b, 0x06, 0x02, 0x00, 0x01, 0xac, 0x60, 0x9d};

    const byte *p_cmd_u8 = NULL;
    uint8 cmd_size_u8 = 0;

    switch (cmd_e)
    {
    case DC_CMD_WAKEUP:
    {
        p_cmd_u8 = CMD_WAKEUP;
        cmd_size_u8 = sizeof(CMD_WAKEUP);
        break;
    }
    case DC_CMD_UP:
    {
        p_cmd_u8 = CMD_UP;
        cmd_size_u8 = sizeof(CMD_UP);
        break;
    }
    case DC_CMD_DOWN:
    {
        p_cmd_u8 = CMD_DOWN;
        cmd_size_u8 = sizeof(CMD_DOWN);
        break;
    }
    case DC_CMD_M:
    {
        p_cmd_u8 = CMD_M;
        cmd_size_u8 = sizeof(CMD_M);
        break;
    }
    case DC_CMD_PRESET_1:
    {
        p_cmd_u8 = CMD_PRESET_1;
        cmd_size_u8 = sizeof(CMD_PRESET_1);
        break;
    }
    case DC_CMD_PRESET_2:
    {
        p_cmd_u8 = CMD_PRESET_2;
        cmd_size_u8 = sizeof(CMD_PRESET_2);
        break;
    }
    case DC_CMD_PRESET_3:
    {
        p_cmd_u8 = CMD_PRESET_3;
        cmd_size_u8 = sizeof(CMD_PRESET_3);
        break;
    }
    case DC_CMD_PRESET_4:
    {
        p_cmd_u8 = CMD_PRESET_4;
        cmd_size_u8 = sizeof(CMD_PRESET_4);
        break;
    }
    default:
    {
        break;
    }
    }

    if ((p_cmd_u8 != NULL) && (cmd_size_u8 > 0))
    {
        dc_request_activation();

        log_buffer(LOG_LEVEL_INFO, dc_module_str, "Sending command", p_cmd_u8, cmd_size_u8);

        dc_serial.flush();
        dc_serial.enableTx(true);
        dc_serial.write(p_cmd_u8, (size_t)cmd_size_u8);
        dc_serial.enableTx(false);
    }
    else
    {
        log_msg(LOG_LEVEL_ERROR, dc_module_str, "Cannot send command of enum index %i.", int(cmd_e));
        ret = -1;
    }

    return ret;
}

void dc_activate()
{
    log_msg(LOG_LEVEL_INFO, dc_module_str, "Activating controller via PIN20.");

    digitalWrite(DC_COMMS_PIN20_U8, HIGH);
    delay(1100);
    digitalWrite(DC_COMMS_PIN20_U8, LOW);
}

uint16 dc_get_current_height()
{
    return dc_state_current_height_u16;
}

DC_STATE dc_get_current_state()
{
    return dc_state_current_e;
}

/*****************************************************************************/

void dc_reset_read_buffer()
{
    (void)memset(dc_rx_buffer_vu8, 0, sizeof(byte) * DC_RX_BUFFER_SIZE);
    dc_rx_buffer_head_u8 = 0U;
}

void dc_reset_current_state()
{
    dc_state_current_height_u16 = 0U;
    dc_state_currently_active_b = false;
}

void dc_handle_serial()
{
    while (dc_serial.available())
    {
        const byte in = dc_serial.read();
        if (in == 0x0)
        {
            continue;
        }

        /* 0x9b == start of packet */
        if (in == 0x9b)
        {
            dc_reset_read_buffer();
        }
        else if (in == 0x9d)
        {
            /* Final byte, can now try to parse the buffer */
            dc_parse_received_message(dc_rx_buffer_vu8, (uint8)DC_RX_BUFFER_SIZE);
        }
        else
        {
            /* Add to buffer but be defensive, we dont want to crash here. If need be, just keep overwriting the last byte  */
            const uint8_t idx_u8 = (dc_rx_buffer_head_u8 < DC_RX_BUFFER_SIZE) ? dc_rx_buffer_head_u8 : (DC_RX_BUFFER_SIZE - 1U);
            dc_rx_buffer_vu8[idx_u8] = in;
            dc_rx_buffer_head_u8 += (dc_rx_buffer_head_u8 < 255) ? 1U : 0U;
        }
    }
}

void dc_handle_state()
{
    uint16 diff_u16;

    /* Synchronize state to height if we have already received the height */
    if (dc_state_current_height_u16 > 0U)
    {
        /* Check standing state */
        diff_u16 = (dc_state_current_height_u16 > dc_height_standing_u16) ? (dc_state_current_height_u16 - dc_height_standing_u16) : (dc_height_standing_u16 - dc_state_current_height_u16);
        if (diff_u16 <= dc_height_tolerance_u16)
        {
            dc_state_current_e = DC_STATE_STANDING;
        }
        else
        {
            /* Check sitting state */
            diff_u16 = (dc_state_current_height_u16 > dc_height_sitting_u16) ? (dc_state_current_height_u16 - dc_height_sitting_u16) : (dc_height_sitting_u16 - dc_state_current_height_u16);
            if (diff_u16 <= dc_height_tolerance_u16)
            {
                dc_state_current_e = DC_STATE_SITTING;
            }
            else
            {
                /* Keep the previous state, might still be valid? */
            }
        }
    }
    else
    {
        ONCE_EVERY_MS(500);
        dc_send_cmd(DC_CMD_WAKEUP); /* this should allow us to read the height in the next loop calls */
    }
}

void dc_parse_received_message(const byte *p_buffer, const uint8 size_u8)
{
    uint16 height_u16;

    if (size_u8 >= 7U)
    {
        if ((p_buffer[0U] == 0x07) && (p_buffer[1U] == 0x12))
        {
            if ((p_buffer[6U] != 0) && (p_buffer[5U] != 0U) && (p_buffer[4U] != 0U))
            {
                height_u16 = dc_parse_height_from_buffer(p_buffer, size_u8);

                /* Do not overwrite previous height - TODO does that make sense? */
                if (height_u16 > 0U)
                {
                    if (height_u16 != dc_state_current_height_u16)
                    {
                        dc_state_current_height_u16 = height_u16;

                        log_msg(LOG_LEVEL_INFO, dc_module_str, "Got height: %i cm.", height_u16);
                    }
                }
                else
                {
                    /* Error message was already written to Serial */
                }
            }
            else
            {
                /* This is the special "sign-off" when the desk is going to sleep */
                log_msg(LOG_LEVEL_INFO, dc_module_str, "Received sign-off, screen is inactive again.");
                dc_state_currently_active_b = false;
            }
        }
        else
        {
            /* Not a height message! */
        }
    }
    else
    {
        /* Unknown message as of yet */
        log_buffer(LOG_LEVEL_WARNING, dc_module_str, "Unknown message", p_buffer, size_u8);
    }
}

uint16 dc_parse_height_from_buffer(const byte *p_buffer, const uint8 size_u8)
{
    uint16 height_u16 = 0;

    /* Just a safeguard that the function before handled everything correctly */
    assert(size_u8 >= 7U);                                                       /* buffer large enough */
    assert((p_buffer[0U] == 0x07) && (p_buffer[1U] == 0x12));                    /* is correct message type */
    assert((p_buffer[6U] != 0) && (p_buffer[5U] != 0U) && (p_buffer[4U] != 0U)); /* is valid height message */

    /* Parse our digits given the assertions that this is a height message */
    const uint8 digits_vu8[3] = {
        dc_parse_digit_from_byte(p_buffer[2U]),
        dc_parse_digit_from_byte(p_buffer[3U]),
        dc_parse_digit_from_byte(p_buffer[4U])};

    /* Check for valid format */
    if ((digits_vu8[0U] < 10) && (digits_vu8[1U] < 10) && (digits_vu8[2U] < 10))
    {
        /* Check whether second byte it is a decimal or not */
        if (p_buffer[3U] & (1UL << 7))
        {
            /* We are < 100cm */
            height_u16 = digits_vu8[2U] + (digits_vu8[1U] * 10) + (digits_vu8[0U] * 100);
        }
        else
        {
            /* We are >= 100cm */
            height_u16 = (digits_vu8[2U] * 10) + (digits_vu8[1U] * 100) + (digits_vu8[0U] * 1000);
        }
    }
    else
    {
        log_buffer(LOG_LEVEL_ERROR, dc_module_str, "Parsing height from invalid buffer", p_buffer, size_u8);
    }

    return height_u16;
}

uint8 dc_parse_digit_from_byte(byte bt)
{
    uint8 digit_u8;

    /* Erase highest bit - it is only the decimal point */
    bt &= ~(1UL << 7);

    /* Check based on bitpattern */
    if (bt == 0b111111)
    {
        digit_u8 = 0;
    }
    else if (bt == 0b110)
    {
        digit_u8 = 1;
    }
    else if (bt == 0b1011011)
    {
        digit_u8 = 2;
    }
    else if (bt == 0b1001111)
    {
        digit_u8 = 3;
    }
    else if (bt == 0b1100110)
    {
        digit_u8 = 4;
    }
    else if (bt == 0b1101101)
    {
        digit_u8 = 5;
    }
    else if (bt == 0b1111101)
    {
        digit_u8 = 6;
    }
    else if (bt == 0b0000111)
    {
        digit_u8 = 7;
    }
    else if (bt == 0b1111111)
    {
        digit_u8 = 8;
    }
    else if (bt == 0b1101111)
    {
        digit_u8 = 9;
    }
    else
    {
        /* Signal error */
        digit_u8 = 255;
    }

    return digit_u8;
}

void dc_request_activation()
{
    if (dc_state_currently_active_b == false)
    {
        dc_activate();
        dc_state_currently_active_b = true;
    }
    else
    {
        /* Already active */
    }
}