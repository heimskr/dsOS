#include "hardware/Ports.h"
#include "hardware/Serial.h"

#define COM1 0x3f8

namespace DsOS::Serial {
	bool ready = false;

	bool init() {
		if (ready)
			return true;

		Ports::outb(COM1 + 1, 0x00);    // Disable all interrupts
		Ports::outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		Ports::outb(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
		Ports::outb(COM1 + 1, 0x00);    //                  (hi byte)
		Ports::outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
		Ports::outb(COM1 + 2, 0xc7);    // Enable FIFO, clear them, with 14-byte threshold
		Ports::outb(COM1 + 4, 0x0b);    // IRQs enabled, RTS/DSR set
		Ports::outb(COM1 + 4, 0x1e);    // Set in loopback mode, test the serial chip
		Ports::outb(COM1 + 0, 0xae);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
		
		// Check if serial is faulty (i.e: not same byte as sent)
		if (Ports::inb(COM1 + 0) != 0xae)
			return false;
		
		// If serial is not faulty set it in normal operation mode
		// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
		Ports::outb(COM1 + 4, 0x0f);
		return ready = true;
	}

	unsigned char read() {
		while ((Ports::inb(COM1 + 5) & 1) == 0);
		return Ports::inb(COM1);
	}

	void write(unsigned char data) {
		while ((Ports::inb(COM1 + 5) & 0x20) == 0);
		Ports::outb(COM1, data);
	}
}
