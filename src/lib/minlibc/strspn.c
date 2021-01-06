#include <string.h>

// Thanks to... user1424589 of Stack Overflow for this code.

size_t strspn(const unsigned char *s, const unsigned char *accept) {
    unsigned char table[32] = {0};
    size_t i;
    for (i = 0; accept[i]; ++i)
        table[accept[i] >> 3] |= 1 << (accept[i] % 8);
    for (i = 0; ((table[s[i] >> 3] >> (s[i] % 8)) & 1); ++i);
    return i;
}
