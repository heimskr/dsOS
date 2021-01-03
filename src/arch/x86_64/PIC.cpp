// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include "arch/x86_64/PIC.h"
#include "hardware/Ports.h"

namespace x86_64::PIC {
	void remap(uint8_t offset1, uint8_t offset2) {
		uint8_t a1 = DsOS::Ports::inb(PIC1_DATA);
		uint8_t a2 = DsOS::Ports::inb(PIC2_DATA);
		DsOS::Ports::outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
		DsOS::Ports::outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
		DsOS::Ports::outb(PIC1_DATA, offset1);
		DsOS::Ports::outb(PIC2_DATA, offset2);
		DsOS::Ports::outb(PIC1_DATA, 4);
		DsOS::Ports::outb(PIC2_DATA, 2);
		DsOS::Ports::outb(PIC1_DATA, PIC_ICW4_8086);
		DsOS::Ports::outb(PIC2_DATA, PIC_ICW4_8086);
		DsOS::Ports::outb(PIC1_DATA, a1);
		DsOS::Ports::outb(PIC2_DATA, a2);
	}

	void disable() {
		DsOS::Ports::outb(PIC1_DATA, 0xff);
		DsOS::Ports::outb(PIC2_DATA, 0xff);
	}
}
