#include "arch/x86_64/control_register.h"

namespace x86_64 {
	uint64_t getCR0() {
		uint64_t out;
		asm volatile("mov %%cr0, %0" : "=r"(out));
		return out;
	}

	uint64_t getCR1() {
		uint64_t out;
		asm volatile("mov %%cr1, %0" : "=r"(out));
		return out;
	}

	uint64_t getCR2() {
		uint64_t out;
		asm volatile("mov %%cr2, %0" : "=r"(out));
		return out;
	}

	uint64_t getCR3() {
		uint64_t out;
		asm volatile("mov %%cr3, %0" : "=r"(out));
		return out;
	}

	uint64_t getCR4() {
		uint64_t out;
		asm volatile("mov %%cr4, %0" : "=r"(out));
		return out;
	}
}