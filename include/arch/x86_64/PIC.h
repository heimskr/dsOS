#pragma once

// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include <stdint.h>

namespace x86_64::PIC {
	void remap(uint8_t offset1, uint8_t offset2);
	void disable();
	void setIRQ(uint8_t);
	void clearIRQ(uint8_t);
	void sendNonspecificEOI(uint8_t irq);
	void sendSpecificEOI(uint8_t irq);
	void sendEOI(uint8_t irq);

	constexpr uint8_t PIC1 = 0x20;
	constexpr uint8_t PIC2 = 0xa0;
	constexpr uint8_t PIC1_COMMAND = PIC1;
	constexpr uint8_t PIC2_COMMAND = PIC2;
	constexpr uint8_t PIC1_DATA = PIC1 + 1;
	constexpr uint8_t PIC2_DATA = PIC2 + 1;
	constexpr uint8_t PIC_ICW1_ICW4 = 0x01;
	constexpr uint8_t PIC_ICW1_INIT = 0x10;
	constexpr uint8_t PIC_ICW4_8086 = 0x01;
	constexpr uint8_t PIC1_REMAP_DEST = 0x20;
	constexpr uint8_t PIC2_REMAP_DEST = 0x28;
	constexpr uint8_t PIC_CUTOFF = 8;
	constexpr uint8_t EOI_NONSPECIFIC = 0x20;
	constexpr uint8_t EOI_SPECIFIC = 0x60;
}
