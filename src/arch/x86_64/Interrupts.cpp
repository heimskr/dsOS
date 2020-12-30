#include "arch/x86_64/control_register.h"
#include "arch/x86_64/Interrupts.h"
#include "lib/printf.h"

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
		add(14, &isr_14);
		asm volatile("lidt (%0)" : : "r" (&idt_header));
	}
}

void div0() {
	printf("Division by zero!\n");
	for (;;);
}

void page_interrupt() {
	printf("Page interrupt: 0x%lx\n", x86_64::getCR2());
	for (;;);
}

extern "C" {
	x86_64::IDT::Descriptor idt[x86_64::IDT::SIZE];
	x86_64::IDT::Header idt_header;
}
