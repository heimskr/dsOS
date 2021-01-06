#include <stdlib.h>

int abs(int j) {
	return j < 0? -j : j;
}

long int labs(long int j) {
	return j < 0L? -j : j;
}

long long int llabs(long long int j) {
	return j < 0LL? -j : j;
}
