#include "Kernel.h"
#include "Terminal.h"

namespace DsOS {
	void Kernel::main() {
		Terminal::write("Hello, kernel World!\n");
		uint64_t rcs = 0;
		asm("mov %%cs, %0" : "=r" (rcs));
		Terminal::putChar('0' + (rcs & 3));
		while (1);
	}
}
