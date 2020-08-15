#include "Kernel.h"
#include "Terminal.h"
#include "lib/printf.h"

namespace DsOS {
	void Kernel::main() {
		Terminal::clear();
		Terminal::write("Hello, kernel World!\n");
		uint64_t rcs = 0;
		asm("mov %%cs, %0" : "=r" (rcs));
		printf("Current ring: %d\n", rcs & 3);
		while (1);
	}
}
