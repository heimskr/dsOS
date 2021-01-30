#pragma once

#include <stdint.h>

namespace x86_64::IDT {
	constexpr int SIZE = 256;

	struct Header {
		uint16_t size;
		uint32_t start;
	} __attribute__((packed));
			
	struct Descriptor {
		uint16_t offset_1;
		uint16_t selector;
		uint8_t ist;
		uint8_t type_attr;
		uint16_t offset_2;
		uint32_t offset_3;
		uint32_t zero;
	} __attribute__((packed));

	void add(int index, void (*fn)());
	void init();
	uint8_t reserveUnusedInterrupt();
}

struct interrupt_frame {
  uint64_t rax, rbx, rcx, rdx;
  uint64_t rdi, rsi, rsp, rbp;
  uint64_t r8,  r9,  r10, r11;
  uint64_t r12, r13, r14, r15;
  uint64_t flags;
} __attribute__((packed));

extern "C" {
	extern x86_64::IDT::Descriptor idt[x86_64::IDT::SIZE];
	extern x86_64::IDT::Header idt_header;
	extern void isr_0();
	extern void isr_8();
	extern void isr_13();
	extern void isr_14();
	extern void isr_32();
	extern void isr_33();
	extern void isr_39();
	extern void isr_46();
	extern void isr_47();

	void div0();
	void double_fault();
	void general_protection_fault();
	void page_interrupt();
	void irq1();
	void spurious_interrupt();
	void irq14();
	void irq15();
}
