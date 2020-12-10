#include "arch/x86_64/cpu.h"
#include "lib/string.h"

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

	void cpuid(unsigned value, unsigned leaf, unsigned &eax, unsigned &ebx, unsigned &ecx, unsigned &edx) {
		asm volatile("cpuid" : "=a" (eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a" (value), "c" (leaf));
	}

	int coreCount() {
		char model[13];
		getModel(model);
		unsigned eax, ebx, ecx, edx;
		if (streq(model, "GenuineIntel")) {
			cpuid(4, 0, eax, ebx, ecx, edx);
			return ((eax >> 26) & 0x3f) + 1;
		} else {
			cpuid(0x80000008, 0, eax, ebx, ecx, edx);
			return (ecx & 0xff) + 1;
		}
	}
}
