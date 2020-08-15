#include "lib/string.h"

extern "C" size_t strlen(const char *str) {
	size_t out;
	for (out = 0; str[out]; ++out);
	return out;
}
