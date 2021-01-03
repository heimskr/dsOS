#include "arch/x86_64/control_register.h"
#include "arch/x86_64/Interrupts.h"
#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "Kernel.h"

#define IRETQ asm volatile("iretq")

namespace x86_64::IDT {
	void add(int index, void (*fn)()) {
		uint64_t offset = (uint64_t) fn;
		Descriptor &descriptor = idt[index];
		descriptor.offset_1 = offset & 0xffff;
		descriptor.offset_2 = (offset >> 16) & 0xffff;
		descriptor.offset_3 = (offset >> 32) & 0xffffffff;
		descriptor.ist = 0;
		descriptor.type_attr = 0x8e;
		descriptor.selector = 0 | 0 | (1 << 3);
		descriptor.zero = 0;
	}

	void init() {
		idt_header.size = SIZE * sizeof(Descriptor) - 1;
		idt_header.start = (uint32_t) (((uint64_t) &idt) & 0xffffffff);

		add(0, &isr_0);
		add(8, &isr_8);
		add(13, &isr_13);
		add(14, &isr_14);
		asm volatile("lidt (%0)" :: "r" (&idt_header));
	}
}

void div0() {
	printf("Division by zero!\n");
	for (;;);
}

void double_fault() {
	printf("Double fault :(\n");
	for (;;);
}

void general_protection_fault() {
	printf("General protection fault :(\n");
	for (;;);
}

void page_interrupt() {
	uint64_t address = x86_64::getCR2();
	constexpr int page_size = 4096;
	printf("Page fault: 0x%lx\n", address);
	using PT = x86_64::PageTableWrapper;
	uint16_t  pml4i = PT::getPML4Index(address);
	uint16_t   pdpi = PT::getPDPTIndex(address);
	uint16_t    pdi = PT::getPDTIndex(address);
	uint16_t    pti = PT::getPTIndex(address);
	uint16_t offset = PT::getOffset(address);
	printf("[PML4 %d, PDP %d, PD %d, PT %d, Offset %d]\n", pml4i, pdpi, pdi, pti, offset);

	DsOS::Kernel *kernel = DsOS::Kernel::instance;
	if (!kernel) {
		printf("Kernel is null!\n");
		for (;;);
	}

	x86_64::PageMeta &meta = kernel->pageMeta;
	if (!meta) {
		printf("Kernel pageMeta is invalid!\n");
		for (;;);
	}

	printf("About to assign.\n");

	if (!meta.assign(pml4i, pdpi, pdi, pti)) {
		printf("Couldn't assign a page!\n");
		for (;;);
	}

	printf("Assigned a page!\n");

	kernel->kernelPML4.print(false);

	memset((void *) (address & ~0xfff), 0, page_size);

	printf("===========================\n\n");

	// Check whether the PML4E is valid.
	//   - If not, choose a space for the new PDPT and update the PML4E, then choose a space for a new PDT and update
	//     the PDPE, then choose a space for a new PT and update the PDE. In the PT, add an entry for an unused page.
	//   - If so, check the relevant PDPE in the PDPT the PML4E points to.
	//       - If it's not valid, choose a space for the new PDT and update the PDPE, then choose a space for a new PT
	//         and update the PDE. In the PT, add an entry for an unused page.
	//       - If it's valid, check the relevant PDE in the PDT the PDPE points to.
	//           - If it's not valid, choose a space for the new PT and update the PDE. In the PT, add an entry for an
	//             unused page.
	//           - If it's valid, check the relevant PTE in the PT the PDE points to.
	//               - If it's valid, why is there a page fault?
	//               - If it's not valid, choose an unused page for the PTE.

	// for (;;);
}

extern "C" {
	x86_64::IDT::Descriptor idt[x86_64::IDT::SIZE];
	x86_64::IDT::Header idt_header;
}
