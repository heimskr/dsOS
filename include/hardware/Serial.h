#pragma once

#include "hardware/Ports.h"

namespace Thorn::Serial {
	using Thorn::Ports::port_t;

	constexpr port_t COM1 = 0x3f8;
	extern bool ready;

	bool init(port_t = COM1);
	unsigned char read(port_t = COM1);
	void write(unsigned char, port_t = COM1);
	void write(const char *, port_t = COM1);
}
