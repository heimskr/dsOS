#include <string.h>
#include "memory/memset.h"

int strcspn(const char *s, const char *reject) {
	static unsigned char marker = 255, table[256];

	if (marker == 255) {
		memset(table, 0, 256);
		marker = 1;
	} else
		++marker;

	for (const char *ptr = reject; *ptr != 0; ptr++)
		table[*ptr] = marker;

	size_t i;
	for (i = 0; s[i] != '\0'; ++i)
		if (table[s[i]] == marker)
			break;
	return i;
}
