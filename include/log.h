#pragma once

#include <stdio.h>

typedef enum log_type_enum
{
    debug,
    info,
    warning,
    error,
    none
} log_type;

void log_set_stream(FILE* stream);
void log_set_level(log_type type);
void log_msg(log_type type, const char *format_str, ...);