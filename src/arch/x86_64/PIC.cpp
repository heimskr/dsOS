// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include "arch/x86_64/PIC.h"
#include "hardware/Ports.h"

namespace x86_64::PIC {
	void remap(uint8_t offset1, uint8_t offset2) {
		uint8_t a1 = Thorn::Ports::inb(PIC1_DATA);
		uint8_t a2 = Thorn::Ports::inb(PIC2_DATA);
		Thorn::Ports::outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
		Thorn::Ports::outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
		Thorn::Ports::outb(PIC1_DATA, offset1);
		Thorn::Ports::outb(PIC2_DATA, offset2);
		Thorn::Ports::outb(PIC1_DATA, 4);
		Thorn::Ports::outb(PIC2_DATA, 2);
		Thorn::Ports::outb(PIC1_DATA, PIC_ICW4_8086);
		Thorn::Ports::outb(PIC2_DATA, PIC_ICW4_8086);
		Thorn::Ports::outb(PIC1_DATA, a1);
		Thorn::Ports::outb(PIC2_DATA, a2);
	}

	void disable() {
		Thorn::Ports::outb(PIC1_DATA, 0xff);
		Thorn::Ports::outb(PIC2_DATA, 0xff);
	}

	void setIRQ(uint8_t irq_line) {
		uint16_t port;
		uint8_t value;
		if (irq_line < 8) {
			port = PIC1_DATA;
		} else {
			port = PIC2_DATA;
			irq_line -= 8;
		}
		value = Thorn::Ports::inb(port) | (1 << irq_line);
		Thorn::Ports::outb(port, value);
	}

	void clearIRQ(uint8_t irq_line) {
		uint16_t port;
		uint8_t value;
		if (irq_line < 8) {
			port = PIC1_DATA;
		} else {
			port = PIC2_DATA;
			irq_line -= 8;
		}
		value = Thorn::Ports::inb(port) & ~(1 << irq_line);
		Thorn::Ports::outb(port, value);
	}

	void sendNonspecificEOI(uint8_t irq) {
		Thorn::Ports::outb(irq < PIC_CUTOFF? PIC1 : PIC2, EOI_NONSPECIFIC);
	}

	void sendSpecificEOI(uint8_t irq) {
		if (PIC_CUTOFF <= irq)
			Thorn::Ports::outb(PIC1, EOI_SPECIFIC | 2);
		Thorn::Ports::outb(irq < PIC_CUTOFF? PIC1 : PIC2, EOI_SPECIFIC | (irq % PIC_CUTOFF));
	}

	void sendEOI(uint8_t irq) {
		sendSpecificEOI(irq);
	}
}
