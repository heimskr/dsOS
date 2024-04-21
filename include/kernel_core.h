#ifndef KERNEL_CORE_H_
#define KERNEL_CORE_H_

#include <arch/x86_64/kernel.h>

#define KERNEL_BOOT_STACK_SIZE 0x4000
#define KERNEL_BOOT_STACK_ALIGNMENT 0x1000

#define THORN_PAGE_SIZE 4096

#ifndef __ASSEMBLER__
#include <stdint.h>
extern "C" volatile uint64_t pml4[];
// extern "C" uint64_t low_pdpt[];
// extern "C" uint64_t high_pdpt[];
// extern "C" uint64_t low_page_directory_table[];
// extern "C" uint64_t high_page_directory_table[];
#endif

#endif
