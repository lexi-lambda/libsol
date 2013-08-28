
#ifndef SOLUTILS_H
#define SOLUTILS_H

#include <stdarg.h>

int vasprintf(char **ret, const char *format, va_list args);
int asprintf(char **ret, const char *format, ...);

#endif
