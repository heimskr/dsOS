#ifndef LIB_PRINTF_H_
#define LIB_PRINTF_H_
#include <stdarg.h>
#include <cstddef>
#include <cstdint>

extern "C" {
	extern bool printf_putc;
	int bprintf(const char *format, ...);
	int bserprintf(const char *format, ...);
	int bsprintf(char *out, const char *format, ...);
	int bsnprintf(char *out, size_t max, const char *format, ...);
	int bvsnprintf(char *out, size_t max, const char *format, va_list);
}

#endif
