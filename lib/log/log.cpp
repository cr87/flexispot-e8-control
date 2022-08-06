#include "log.h"

#include <stdarg.h>

#include <SoftwareSerial.h>

/*****************************************************************************/

LOG_LEVEL log_global_level_e = LOG_LEVEL_SILENT;

/*****************************************************************************/

LOG_LEVEL log_get_global_level()
{
    return log_global_level_e;
}

void log_set_global_level(const LOG_LEVEL level_e)
{
    log_global_level_e = level_e;
}

void log_msg(const LOG_LEVEL level_e, const char *module, const char *message, ...)
{
#ifndef DISABLE_LOGGING
    va_list args;

    if (level_e <= log_global_level_e)
    {
        char buffer[256];
        va_start(args, message);
        vsprintf(buffer, message, args);
        Serial.printf("[%s] ", module);
        Serial.println(buffer);
        va_end(args);
    }
    else
    {
    }
#endif
}

void log_buffer(const LOG_LEVEL level_e, const char *module, const char *message, const uint8 *p_buffer, uint8 size_u8)
{
#ifndef DISABLE_LOGGING
    if (level_e <= log_global_level_e)
    {
        Serial.printf("[%s]", module);
        Serial.printf("%s: ", message);
        for (uint8 i = 0; i < size_u8; ++i)
        {
            Serial.printf(" %i", p_buffer[i]);
        }
        Serial.println();
    }
    else
    {
    }
#endif
}

/*****************************************************************************/
