#include "Kernel.h"
#include "Terminal.h"
#include "lib/printf.h"
#include "multiboot2.h"
#include "arch/x86_64/APIC.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/Interrupts.h"

extern void *multiboot_data;
extern unsigned int multiboot_magic;

#define DEBUG_MMAP

namespace DsOS {
	Kernel * Kernel::instance = nullptr;

	Kernel::Kernel(x86_64::PageTable *pml4_): pml4(pml4_) {
		if (Kernel::instance) {
			printf("Kernel instantiated twice!\n");
			for (;;);
		} else
			Kernel::instance = this;
	}

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
		printf("APIC: %s\n", x86_64::checkAPIC()? "yes" : "no");
		printf("Memory: 0x%x through 0x%x\n", memoryLow, memoryHigh);
		printf("Core count: %d\n", x86_64::coreCount());
		printf("ARAT: %s\n", x86_64::arat()? "true" : "false");

		x86_64::IDT::init();
		x86_64::APIC::init();

		int x = *((int *) 0xdeadbeef);
		printf("x = %d\n", x);

		// int x = 6 / 0;
		// for (;;);

		// int *somewhere = new int(42);
		// printf("somewhere: [%ld] = %d\n", somewhere, *somewhere);

		// for (size_t address = (size_t) multiboot_data;; address *= 1.1) {
		// 	Terminal::clear();
		// 	printf("Address: %ld -> %d", address, *((int *) address));
		// 	for (int j = 0; j < 30000000; ++j);
		// }
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
					// printf("mmap\n");
					for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
						(multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size;
						mmap = (multiboot_memory_map_t *)
								((unsigned long) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size)) {
						// printf(" base_addr = 0x%lx, length = 0x%lx, type = %u\n", mmap->addr, mmap->len, mmap->type);
					}
					break;
				}
#endif
			}
		}

		if (memoryLow != 0 && memoryHigh != 0) {
			printf("Resetting memory bounds: %ld, %llu.\n", memoryLow, memoryHigh);
			memory.setBounds((char *) memoryLow, (char *) memoryHigh);
		}
	}
}
