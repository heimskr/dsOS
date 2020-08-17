#include "arch/x86_64/cpu.h"

namespace x86_64 {
	void getModel(char *out) {
		size_t ebx, ecx, edx, _;
		__cpuid(0, _, ebx, ecx, edx);
		unsigned char i = 0;
		while (ebx)
			out[i++] = ebx & 0xff, ebx >>= 8;
		while (edx)
			out[i++] = edx & 0xff, edx >>= 8;
		while (ecx)
			out[i++] = ecx & 0xff, ecx >>= 8;
		out[i] = '\0';
	}

	int checkAPIC() {
		unsigned int edx, _;
		__get_cpuid(1, &_, &_, &_, &edx);
		return !!(edx & (1 << 9));
	}
}
