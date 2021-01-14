#pragma once

#include <cstdint>

namespace DsOS::Ports {
	using port_t = uint16_t;

	uint8_t inb(port_t);
	void outb(port_t, uint8_t);

	uint16_t inw(port_t);
	void outw(port_t, uint16_t);

	uint32_t inl(port_t);
	void outl(port_t, uint32_t);
}
