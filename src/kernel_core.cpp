#include "Kernel.h"
#include "Terminal.h"

#if defined(__linux__)
#error "You are not using a cross-compiler. You will most certainly run into trouble."
#endif

#if !defined(__x86_64__)
#error "The kernel needs to be compiled with an x86_64-elf compiler."
#endif

void kernel_main(void) {
	DsOS::Kernel kernel;
	kernel.main();
}
