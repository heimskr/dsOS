#pragma once

namespace DsOS::Serial {
	extern bool ready;
	bool init();
	unsigned char read();
	void write(unsigned char);
}
