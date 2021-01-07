#include "arch/x86_64/APIC.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/Interrupts.h"
#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "arch/x86_64/PIC.h"
#include "hardware/Ports.h"
#include "hardware/PS2Keyboard.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "Kernel.h"

#define INTERRUPT 

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
		add(32, &isr_32);
		add(33, &isr_33);
		add(39, &isr_39);
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


extern uint64_t gpf_addr;

void general_protection_fault() {
	printf("General protection fault caused by 0x%lx\n", gpf_addr);
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

	x86_64::PageMeta &meta = kernel->pager;
	if (!meta) {
		printf("Kernel pager is invalid!\n");
		kernel->backtrace();
		for (;;);
	}

	uintptr_t assigned = meta.assign(pml4i, pdpi, pdi, pti) & ~0xfff;

	if (assigned == 0) {
		printf("Couldn't assign a page!\n");
		for (;;);
	}

	printf("Assigned a page (0x%lx)!\n", assigned);

	// kernel->kernelPML4.print(false);

	// asm volatile("clflush (%0)" :: "r"(assigned));
	// memset((void *) (address & ~0xfff), '\0', page_size);
	printf("[:\n");
	// for (size_t i = 0; i < page_size / sizeof(uint64_t); ++i) {
	// 	*((uint64_t *) ((address & ~0xfff) + i)) = 0;
	// }

	printf(":]\n");
	// asm volatile("wbinvd");
}

void spurious_interrupt() {
	printf("Spurious interrupt\n");
}

void irq1() {
	printf("what\n");
	DsOS::PS2Keyboard::onIRQ1();
}

extern "C" {
	x86_64::IDT::Descriptor idt[x86_64::IDT::SIZE];
	x86_64::IDT::Header idt_header;
}
