#include "hardware/Ports.h"

namespace DsOS::Ports {
	uint8_t inb(port_t port) {
		uint8_t out = 0;
		asm volatile("inb %1, %0" : "=a"(out) : "dN"(port));
		return out;
	}

	void outb(port_t port, uint8_t data) {
		asm volatile("outb %1, %0" :: "dN"(port), "a"(data));
	}

	uint16_t inw(port_t port) {
		uint16_t out = 0;
		asm volatile("inw %1, %0" : "=a"(out) : "dN"(port));
		return out;
	}

	void outw(port_t port, uint16_t data) {
		asm volatile("outw %1, %0" :: "dN"(port), "a"(data));
	}

	uint32_t inl(port_t port) {
		uint32_t out = 0;
		asm volatile("inl %1, %0" : "=a"(out) : "dN"(port));
		return out;
	}

	void outl(port_t port, uint32_t data) {
		asm volatile("outl %1, %0" :: "dN"(port), "a"(data));
	}
}
