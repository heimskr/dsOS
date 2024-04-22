#ifndef KERNEL_CORE_H_
#define KERNEL_CORE_H_

#include "arch/x86_64/kernel.h"

#define KERNEL_BOOT_STACK_SIZE 0x4000
#define KERNEL_BOOT_STACK_ALIGNMENT 0x1000

#define THORN_PAGE_SIZE (4 << 10)

#ifndef __ASSEMBLER__
#include <stdint.h>
extern "C" volatile uint64_t pml4[];
#endif

#endif
