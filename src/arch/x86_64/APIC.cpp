// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include "arch/x86_64/APIC.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/PIC.h"
#include "lib/printf.h"
#include "Kernel.h"

namespace x86_64::APIC {
	void init(DsOS::Kernel &kernel) {
		printf("Initializing APIC timer.\n");
		uint64_t msr = rdmsr(APIC_MSR);
		uint32_t *apic_base;
		apic_base = (uint32_t *) (msr & 0xffffff000);
		wrmsr(APIC_MSR, msr | APIC_ENABLE, msr >> 32);
		printf("APIC base: 0x%lx, ID: 0x%lx\n", apic_base, apic_base + APIC_REGISTER_APICID);
		kernel.pageMeta.identityMap((void *) apic_base);
		apic_base[APIC_REGISTER_SPURIOUS] = APIC_SPURIOUS_ENABLE | BSP_VECTOR_SPURIOUS;
		PIC::remap(PIC::PIC1_REMAP_DEST, PIC::PIC2_REMAP_DEST);
		PIC::disable();
	}
}
