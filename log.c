#include "log.h"

#include <stdarg.h>
#include <stdio.h>

void log_error(const char *ident, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    fputs("ERROR: ", stderr);
    vfprintf(stderr, format, ap);
    va_end(ap);
}
