#include "log.h"

#include <stdarg.h>
#include <string.h>

static FILE *log_fstream = NULL;
static log_type log_level = LOG_WRN;

const char *LOG_TYP_STR[] = {"DBG", "INF", "WRN", "ERR", "NON"};

void log_set_stream(FILE *stream)
{
    log_fstream = stream;
}

void log_set_level(log_type type)
{
    log_level = type;
}

void log_msg(log_type type, const char *format_str, ...)
{
    if (type < log_level)
        return;

    if (!log_fstream)
    {
        if (type >= LOG_WRN)
            log_fstream = stderr;
        else
            log_fstream = stdout;
    }

    va_list args;
    char frmt[LOG_MAX_LEN + 1] = {0};
    strcat(frmt, LOG_TYP_STR[type]);
    strcat(frmt, ": ");
    strncat(frmt, format_str, (LOG_MAX_LEN - 5));
    va_start(args, format_str);
    vfprintf(log_fstream, frmt, args);
    va_end(args);
}
