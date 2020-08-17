#include "arch/x86_64/control_register.h"

namespace x86_64 {
	size_t getCR0() {
		size_t out;
		asm volatile("mov %%cr0, %0" : "=r"(out));
		return out;
	}

	size_t getCR1() {
		size_t out;
		asm volatile("mov %%cr1, %0" : "=r"(out));
		return out;
	}

	size_t getCR2() {
		size_t out;
		asm volatile("mov %%cr2, %0" : "=r"(out));
		return out;
	}

	size_t getCR3() {
		size_t out;
		asm volatile("mov %%cr3, %0" : "=r"(out));
		return out;
	}

	size_t getCR4() {
		size_t out;
		asm volatile("mov %%cr4, %0" : "=r"(out));
		return out;
	}
}