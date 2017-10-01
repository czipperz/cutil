#include "log.h"
#include <stdarg.h>
#include <stdio.h>

static log_importance minimum_importance = log_importance_warning;

void log_set_minimum_importance(log_importance importance) {
    minimum_importance = importance;
}

void log_(const char* filename, unsigned long line,
          log_importance importance, const char* format, ...) {
    va_list list;
    if (importance >= minimum_importance) {
        printf("%s:%lu: ", filename, line);
        switch (importance) {
        case log_importance_debug:
            fputs("debug: ", stdout);
            break;
        case log_importance_warning:
            fputs("warning: ", stdout);
            break;
        case log_importance_error:
            fputs("error: ", stdout);
            break;
        }
        va_start(list, format);
        vprintf(format, list);
        va_end(list);
        putchar('\n');
    }
}
