#ifndef LIB_PRINTF_H_
#define LIB_PRINTF_H_
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

extern "C" {
	int printf(const char *format, ...);
	int sprintf(char *out, const char *format, ...);
	int snprintf(char *out, const size_t max, const char *format, ...);
	int vsnprintf(char *out, const size_t max, const char *format, va_list);
}


#endif
