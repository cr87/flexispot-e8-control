#ifndef LOG_H_

/*****************************************************************************/

#include <c_types.h>

/*****************************************************************************/

typedef enum
{
    LOG_LEVEL_SILENT = 0,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} LOG_LEVEL;

/*****************************************************************************/

extern LOG_LEVEL log_get_global_level();
extern void log_set_global_level(const LOG_LEVEL level_e);

extern void log_msg(const LOG_LEVEL level_e, const char *module, const char *message, ...);
extern void log_buffer(const LOG_LEVEL level_e, const char *module, const char *message, const uint8 *p_buffer, uint8 size_u8);

/*****************************************************************************/

#endif