#include "lib/string.h"

extern "C" size_t strlen(const char *str) {
	size_t out;
	for (out = 0; str[out]; ++out);
	return out;
}

extern "C" bool streq(const char *one, const char *two) {
	if (strlen(one) != strlen(two))
		return false;
	for (int i = 0; one[i] != '\0'; ++i)
		if (one[i] != two[i])
			return false;
	return true;
}
