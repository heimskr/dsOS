#include "lib/string.h"

extern "C" {
	size_t strlen(const char *str) {
		size_t out;
		for (out = 0; str[out]; ++out);
		return out;
	}

	bool streq(const char *one, const char *two) {
		if (strlen(one) != strlen(two))
			return false;
		for (int i = 0; one[i] != '\0'; ++i)
			if (one[i] != two[i])
				return false;
		return true;
	}

	char * strcpy(char *dest, const char *src) {
		size_t i;
		for (i = 0; src[i]; ++i)
			dest[i] = src[i];
		dest[i] = '\0';
		return dest;
	}

	char * strncpy(char *dest, const char *src, size_t n) {
		size_t i;
		for (i = 0; i < n && src[i]; ++i)
			dest[i] = src[i];
		for (; i < n; ++i)
			dest[i] = '\0';
		return dest;
	}
}
