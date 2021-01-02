#include "Kernel.h"
#include "Terminal.h"
#include "multiboot2.h"

#if defined(__linux__)
#error "You are not using a cross-compiler. You will most certainly run into trouble."
#endif

#if !defined(__x86_64__)
#error "The kernel needs to be compiled with an x86_64-elf compiler."
#endif

namespace x86_64 {
	struct PageTable;
}

extern "C" void kernel_main() {
	DsOS::Kernel kernel(x86_64::PageTableWrapper(&pml4, x86_64::PageTableWrapper::Type::PML4));
	kernel.main();
}
