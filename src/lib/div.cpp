#include <stdlib.h>

div_t div(int numerator, int denominator) {
	return {
		.quot = numerator / denominator,
		.rem  = numerator % denominator
	};
}

ldiv_t ldiv(long numerator, long denominator) {
	return {
		.quot = numerator / denominator,
		.rem  = numerator % denominator
	};
}

lldiv_t lldiv(long long numerator, long long denominator) {
	return {
		.quot = numerator / denominator,
		.rem  = numerator % denominator
	};
}
