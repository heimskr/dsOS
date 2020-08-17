#include "Kernel.h"
#include "Terminal.h"
#include "lib/printf.h"
#include "multiboot2.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/cpu.h"

extern void *multiboot_data;
extern unsigned int multiboot_magic;

namespace DsOS {
	void Kernel::main() {
		Terminal::clear();
		Terminal::write("Hello, kernel World!\n");
		uint64_t rcs = 0, pfla = 0;
		asm("mov %%cs, %0" : "=r" (rcs));
		printf("Current ring: %d\n", rcs & 3);
		asm("mov %%cr2, %0" : "=r" (pfla));
		printf("PFLA: %llu\n", pfla);
		detectMemory();
		printf("CR0: %x, CR2: %x, CR3: %x, CR4: %x\n", x86_64::getCR0(), x86_64::getCR2(), x86_64::getCR3(), x86_64::getCR4());
		char model[13];
		x86_64::getModel(model);
		printf("Model: %s\n", model);
		printf("APIC: %d\n", x86_64::checkAPIC());
		printf("Memory: 0x%x through 0x%x\n", memoryLow, memoryHigh);
		while (1);
	}

	void Kernel::detectMemory() {
		struct multiboot_tag *tag;
		unsigned long addr = (unsigned long) multiboot_data;

		if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
			printf("Invalid magic number: %d\n", (unsigned) multiboot_magic);
			return;
		}

		if (addr & 7) {
			printf("Unaligned mbi: %d\n", addr);
			return;
		}

		for (tag = (struct multiboot_tag *) (addr + 8);
			tag->type != MULTIBOOT_TAG_TYPE_END;
			tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))) {

			switch (tag->type) {
				case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
					memoryLow  = ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower * 1024;
					memoryHigh = ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper * 1024;
#ifdef DEBUG_MMAP
					printf("mem_lower = %uKB, mem_upper = %uKB\n",
						((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
						((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
#endif
					break;
#ifdef DEBUG_MMAP
				case MULTIBOOT_TAG_TYPE_MMAP: {
					multiboot_memory_map_t *mmap;
					printf("mmap\n");
					for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
						(multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size;
						mmap = (multiboot_memory_map_t *)
								((unsigned long) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size)) {
						printf(" base_addr = 0x%lx, length = 0x%lx, type = %u\n", mmap->addr, mmap->len, mmap->type);
					}
					break;
				}
#endif
			}
		}
	}
}
