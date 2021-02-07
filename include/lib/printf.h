#ifndef LIB_PRINTF_H_
#define LIB_PRINTF_H_
#include <stdarg.h>
#include <cstddef>
#include <cstdint>

extern "C" {
	extern bool printf_putc;
	int printf(const char *format, ...);
	int sprintf(char *out, const char *format, ...);
	int snprintf(char *out, size_t max, const char *format, ...);
	int vsnprintf(char *out, size_t max, const char *format, va_list);
}



#endif
