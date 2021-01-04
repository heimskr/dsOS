#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>

extern "C" {
	size_t strlen(const char *);
	bool streq(const char *, const char *);
	char * strcpy(char *dest, const char *src);
	char * strncpy(char *dest, const char *src, size_t n);
}

#endif
