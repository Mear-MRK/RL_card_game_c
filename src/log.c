#include "log.h"

#include <stdarg.h>

static FILE *log_fstream = NULL;
static log_type log_level = warning;

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
    
    if(!log_fstream)
        log_fstream = stdout;
    va_list args;
    va_start(args, format_str);
    vfprintf(log_fstream, format_str, args);
    va_end(args);
}
