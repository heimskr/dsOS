#include <cstddef>

#include "hardware/Ports.h"
#include "hardware/Serial.h"


namespace Thorn::Serial {
	using Thorn::Ports::port_t;
	bool ready = false;

	bool init(port_t base) {
		if (base == COM1 && ready)
			return true;

		Ports::outb(base + 1, 0x00); // Disable all interrupts
		Ports::outb(base + 3, 0x80); // Enable DLAB (set baud rate divisor)
		Ports::outb(base + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
		Ports::outb(base + 1, 0x00); //                  (hi byte)
		Ports::outb(base + 3, 0x03); // 8 bits, no parity, one stop bit
		Ports::outb(base + 2, 0xc7); // Enable FIFO, clear them, with 14-byte threshold
		Ports::outb(base + 4, 0x0b); // IRQs enabled, RTS/DSR set
		Ports::outb(base + 4, 0x1e); // Set in loopback mode, test the serial chip
		Ports::outb(base + 0, 0xae); // Test serial chip (send byte 0xAE and check if serial returns same byte)

		// Check if serial is faulty (i.e.: not same byte as sent)
		if (Ports::inb(base + 0) != 0xae)
			return false;

		// If serial is not faulty set it in normal operation mode
		// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
		Ports::outb(base + 4, 0x0f);
		if (base == COM1)
			return ready = true;
		return true;
	}

	unsigned char read(port_t base) {
		while ((Ports::inb(base + 5) & 1) == 0);
		return Ports::inb(base);
	}

	void write(unsigned char data, port_t base) {
		while ((Ports::inb(base + 5) & 0x20) == 0);
		Ports::outb(base, data);
	}

	void write(const char *str, port_t base) {
		for (size_t i = 0; str[i] != '\0'; ++i)
			write((unsigned char) str[i], base);
	}
}
