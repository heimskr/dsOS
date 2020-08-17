#include <cpuid.h>
#include <stddef.h>

namespace x86_64 {
	void getModel(char *);
	int checkAPIC();
}
