#pragma once

#include <stdint.h>

namespace x86_64 {
	void cpuid(uint32_t value, uint32_t leaf, uint32_t &eax, uint32_t &ebx, uint32_t &ecx, uint32_t &edx);
	void getModel(char *);
	bool checkAPIC();
	int coreCount();
	bool arat();

	inline void wrmsr(uint32_t reg, uint32_t low, uint32_t high) {
		asm volatile("wrmsr" :: "a" (low), "d" (high), "c" (reg));
	}

	inline uint64_t rdmsr(uint32_t reg) {
		uint32_t low, high;
		asm volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (reg));
		return (static_cast<uint64_t>(high) << 32) | low;
	}

	inline void enableInterrupts() {
		asm volatile("sti");
	}

	inline void disableInterrupts() {
		asm volatile("cli");
	}
}
