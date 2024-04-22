#include "Kernel.h"

extern "C" void kernel_main() {
	Thorn::Kernel kernel(x86_64::PageTableWrapper(pml4, x86_64::PageTableWrapper::Type::PML4));
	kernel.main();
}
