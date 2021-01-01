#include "hardware/Ports.h"

namespace DsOS::Ports {
	unsigned char inb(unsigned short port) {
		unsigned char out = 0;
		asm volatile("inb %1, %0" : "=a"(out) : "dN"(port));
		return out;
	}

	void outb(unsigned short port, unsigned char data) {
		asm volatile("outb %1, %0" :: "dN"(port), "a"(data));
	}
}
