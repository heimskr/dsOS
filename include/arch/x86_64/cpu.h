#include <cpuid.h>
#include <stddef.h>

namespace x86_64 {
	void getModel(char *);
	int checkAPIC();
	void cpuid(unsigned value, unsigned leaf, unsigned &eax, unsigned &ebx, unsigned &ecx, unsigned &edx);
	int coreCount();
}
