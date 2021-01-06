#include <string.h>

size_t strxfrm(char *dest, const char *src, size_t n) {
	/// TODO: implement locales?
	size_t i = 0;
	for (i = 0; src[i] != '\0' && i < n; ++i)
		dest[i] = src[i];
	return 0 < i? i - 1 : 0;
}
