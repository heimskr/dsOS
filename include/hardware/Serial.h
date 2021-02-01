#pragma once

namespace Thorn::Serial {
	extern bool ready;
	bool init();
	unsigned char read();
	void write(unsigned char);
	void write(const char *);
}
