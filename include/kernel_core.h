#ifndef KERNEL_CORE_H_
#define KERNEL_CORE_H_

#include <arch/x86_64/kernel.h>

#define KERNEL_BOOT_STACK_SIZE 0x4000
#define KERNEL_BOOT_STACK_ALIGNMENT 0x1000

#ifndef __ASSEMBLER__
extern "C" void *pml4;
extern "C" void *detected_memory;
extern "C" void *low_pdpt;
extern "C" void *high_pdpt;
extern "C" void *low_page_directory_table;
extern "C" void *high_page_directory_table;
#endif

#endif
