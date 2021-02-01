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
#include "Options.h"

#define INTERRUPT 

// #define DEBUG_PAGE_FAULTS

extern bool irqInvoked;

namespace x86_64::IDT {
	void add(int index, void (*fn)()) {
		uintptr_t offset = (uintptr_t) fn;
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
		idt_header.start = (uint32_t) (((uintptr_t) &idt) & 0xffffffff);
		memset(idt, 0, sizeof(idt));

		add(0, &isr_0);
		add(8, &isr_8);
		add(13, &isr_13);
		add(14, &isr_14);
		add(32, &isr_32);
		add(33, &isr_33);
		add(39, &isr_39);
		add(46, &isr_46);
		add(47, &isr_47);
		asm volatile("lidt %0" :: "m"(idt_header));
	}

	uint8_t reserveUnusedInterrupt() {
		for (uint8_t i = 48; i < 255; ++i)
			if (idt[i].type_attr == 0) {
				add(i, +[] { printf("Invalid interrupt handler called!\n"); });
				return i;
			}
		return 0xff;
	}
}

void div0() {
	printf("Division by zero!\n");
	for (;;) asm("hlt");
}

void double_fault() {
	printf("Double fault :(\n");
	for (;;) asm("hlt");
}

void page_interrupt() {
	const uint64_t address = x86_64::getCR2();
	constexpr int page_size = 4096;
#ifdef DEBUG_PAGE_FAULTS
	printf("Page fault: 0x%lx\n", address);
#endif
	using PT = x86_64::PageTableWrapper;
	uint16_t  pml4i = PT::getPML4Index(address);
	uint16_t   pdpi = PT::getPDPTIndex(address);
	uint16_t    pdi = PT::getPDTIndex(address);
	uint16_t    pti = PT::getPTIndex(address);
	uint16_t offset = PT::getOffset(address);
	(void) offset;
#ifdef DEBUG_PAGE_FAULTS
	printf("[PML4 %d, PDP %d, PD %d, PT %d, Offset %d]\n", pml4i, pdpi, pdi, pti, offset);
#endif

	Thorn::Kernel *kernel = Thorn::Kernel::instance;
	if (!kernel) {
		printf("Kernel is null!\n");
		for (;;) asm("hlt");
	}

	x86_64::PageMeta &meta = kernel->pager;
	if (!meta) {
		printf("Kernel pager is invalid!\n");
		kernel->backtrace();
		for (;;) asm("hlt");
	}

	uintptr_t assigned = meta.assign(pml4i, pdpi, pdi, pti) & ~0xfff;

	if (assigned == 0) {
		printf("Couldn't assign a page! [%d, %d, %d, %d]\n", pml4i, pdpi, pdi, pti);
		for (;;) asm("hlt");
	}

	uint64_t *base = (uint64_t *) (address & ~0xfff);
	for (size_t i = 0; i < page_size / sizeof(uint64_t); ++i)
		base[i] = 0;

#ifdef DEBUG_PAGE_FAULTS
	printf("Assigned a page (0x%lx)!\n", assigned);
#endif
}

void spurious_interrupt() {
	printf("Spurious interrupt\n");
}

void irq1() {
	Thorn::PS2Keyboard::onIRQ1();
}

void irq14() {
	Thorn::Ports::outb(0xa0, 0x20);
	x86_64::PIC::sendEOI(1);
	printf("IRQ14\n");
	for (;;) asm("hlt");
	// irqInvoked = 1;
}

void irq15() {
	Thorn::Ports::outb(0xa0, 0x20);
	x86_64::PIC::sendEOI(1);
	printf("IRQ15\n");
	for (;;) asm("hlt");
	// irqInvoked = 1;
}

extern "C" {
	x86_64::IDT::Descriptor idt[x86_64::IDT::SIZE];
	x86_64::IDT::Header idt_header;
}
