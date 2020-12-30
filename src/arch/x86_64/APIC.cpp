#include "arch/x86_64/CPU.h"
#include "arch/x86_64/APIC.h"
#include "lib/printf.h"

namespace x86_64::APIC {
	void init() {
		printf("Initializing APIC timer.\n");
		uint64_t msr = rdmsr(APIC_MSR);
		uint32_t *apic_base = (uint32_t *) (msr & 0xffffff000);
		wrmsr(APIC_MSR, msr | APIC_ENABLE, msr >> 32);
		printf("APIC base: 0x%lx, ID: 0x%lx\n", apic_base, apic_base + APIC_REGISTER_APICID);
	}
}
