#include <string.h>

char * strncat(char *dest, const char *src, size_t n) {
	size_t i, length = strlen(dest);
	for (i = 0 ; src[i] != '\0' && i < n; i++)
		dest[length + i] = src[i];
	dest[length + i] = '\0';
	return dest;
}
