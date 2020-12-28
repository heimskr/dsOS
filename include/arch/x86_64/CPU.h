#pragma once

#include <stddef.h>

namespace x86_64 {
	void cpuid(unsigned value, unsigned leaf, unsigned &eax, unsigned &ebx, unsigned &ecx, unsigned &edx);
	void getModel(char *);
	bool checkAPIC();
	int coreCount();
	bool arat();
}
