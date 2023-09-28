#pragma once

#include <stdio.h>

typedef enum log_type_enum
{
    LOG_DBG = 0,    // debug
    LOG_INF,        // info
    LOG_WRN,        // warning
    LOG_ERR,        // error
    LOG_NON         // none (no log limit)
} log_type;

#define LOG_MAX_LEN 127

extern const char* LOG_TYP_STR[];

void log_set_stream(FILE* stream);
void log_set_level(log_type type);
void log_msg(log_type type, const char *format_str, ...);