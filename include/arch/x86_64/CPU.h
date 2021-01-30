#pragma once

// Some code is from https://github.com/fido2020/Lemon-OS/blob/master/Kernel/include/arch/x86_64/cpu.h

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

	inline bool checkInterrupts() {
		unsigned long flags;
		asm volatile("pushf; pop %%rax" : "=a"(flags) :: "cc");
		return flags & 0x200;
	}

	struct GDTPointer {
		uint16_t limit;
		uint64_t base;
	} __attribute__((packed));

	struct CPU {
		CPU *self;
		uint64_t id; // APIC/CPU id
		void *gdt;
		GDTPointer gdtPointer;
		// thread_t* currentThread = nullptr;
		// process* idleProcess = nullptr;
		// volatile int runQueueLock = 0;
		// FastList<thread_t*>* runQueue;
		// tss_t tss __attribute__((aligned(16)));
	};

	__attribute__((always_inline)) inline CPU * getCPULocal() {
		CPU *out;
		bool interrupts = checkInterrupts();
		asm("cli");
		asm volatile("swapgs; movq %%gs:0, %0; swapgs" : "=r"(out));
		if (interrupts)
			asm("sti");
		return out;
	}
}
