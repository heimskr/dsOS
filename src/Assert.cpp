#include "Assert.h"
#include "lib/printf.h"

void assert_(bool condition, const char *condstr, const char *file, int line) {
	if (!condition) {
		printf("Assertion %s failed at %s:%d!\n", condstr, file, line);
		for (;;)
			asm("hlt");
	}
}
