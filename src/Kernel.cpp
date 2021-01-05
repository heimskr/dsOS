#include "Kernel.h"
#include "Terminal.h"
#include "DsUtil.h"
#include "hardware/Serial.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "multiboot2.h"
#include "arch/x86_64/APIC.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/Interrupts.h"
#include "arch/x86_64/PIC.h"
#include "lib/string"

extern void *multiboot_data;
extern unsigned int multiboot_magic;

extern void *tmp_stack;

#define DEBUG_MMAP

extern uint64_t ticks;

// void timer_test() {
// 	printf("Timer done.\n");
// }

namespace DsOS {
	Kernel * Kernel::instance = nullptr;

	Kernel::Kernel(const x86_64::PageTableWrapper &pml4_): kernelPML4(pml4_) {
		if (Kernel::instance) {
			printf("Kernel instantiated twice!\n");
			for (;;);
		} else
			Kernel::instance = this;
	}

	void Kernel::main() {
		Terminal::clear();
		Terminal::write("Hello, kernel World!\n");
		if (Serial::init())
			for (char ch: "\n\n\n")
				Serial::write(ch);
		uint64_t rcs = 0, pfla = 0;
		asm("mov %%cs, %0" : "=r" (rcs));
		// printf("Current ring: %d\n", rcs & 3);
		asm("mov %%cr2, %0" : "=r" (pfla));
		// printf("PFLA: %llu\n", pfla);
		detectMemory();
		arrangeMemory();
		initPageDescriptors();

		// pml4->print();

		printf("Kernel: 0x%lx\n", (uintptr_t) this);

		// printf("CR0: %x, CR2: %x, CR3: %x, CR4: %x\n", x86_64::getCR0(), x86_64::getCR2(), x86_64::getCR3(), x86_64::getCR4());
		// char model[13];
		// x86_64::getModel(model);
		// printf("Model: %s\n", model);
		// printf("APIC: %s\n", x86_64::checkAPIC()? "yes" : "no");
		printf("Memory: 0x%x through 0x%x\n", memoryLow, memoryHigh);
		// printf("Core count: %d\n", x86_64::coreCount());
		// printf("ARAT: %s\n", x86_64::arat()? "true" : "false");

		x86_64::IDT::init();

		pager = x86_64::PageMeta4K((void *) 0x800000UL, (void *) 0xffff80800000UL, (void *) 0x600000UL, (memoryHigh - 0x800000UL) / 4096);
		pager.assignSelf();
		pager.clear();

		memory.setBounds((char *) 0xfffff00000000000UL, (char *) 0xfffffffffffff000UL);

		x86_64::APIC::init(*this);

		printf("pageDescriptors: 0x%lx\n", (uintptr_t) pageDescriptors);
		printf("pageDescriptorsLength: 0x%lx\n", pageDescriptorsLength);
		printf("pagesStart: 0x%lx\n", (uintptr_t) pagesStart);
		printf("pagesLength: 0x%lx\n", pagesLength);
		printf("&tmp_stack: 0x%lx\n", &tmp_stack);

		int *somewhere = new int(42);
		printf("somewhere: [0x%lx] = %d\n", somewhere, *somewhere);

		printf("sizeof(PageMeta) = %ld, sizeof(PageMeta4K) = %ld\n", sizeof(x86_64::PageMeta), sizeof(x86_64::PageMeta4K));
		printf("pageCount = %d, bitmapSize = %ld\n", pager.pageCount(), pager.bitmapSize());

		kernelPML4.print(false);
		// printf(" ------------------------------------------------------------------------------\n");

		// for (uint64_t i = 0;; ++i)
		// 	int x = *((uint64_t *) i);

		// printf("<%d> : <%d>\n", pager.findFree(), pager.pagesUsed());
		// printf("<%d> : ", pager.findFree());
		// printf("<%d>\n", pager.pagesUsed());

		int x = *((int *) 0xdeadbeef);
		printf("x = %d\n", x);

		// printf("<%d> : <%d>\n", pager.findFree(), pager.pagesUsed());

		// x86_64::APIC::initTimer(1);
		// while (ticks != 5);
		// x86_64::APIC::initTimer(5);
		// while (ticks != 20);
		// x86_64::APIC::disableTimer();
		// wait(1000);
		// printf("Hello.\n");
		// x86_64::APIC::initTimer(5);

		x86_64::PIC::clearIRQ(1);

		x86_64::APIC::initTimer(2);
		x86_64::APIC::disableTimer();

		std::string foo = "hello";
		foo.append(" friend");
		foo.insert(5, " there");
		foo.replace(1, 4, "i");
		for (auto iter = foo.begin(), end = foo.end(); iter != end; ++iter)
			printf("[%c] %d\n", *iter, *iter);
		printf("[%s]\n", foo.c_str());
		foo.replace(1, 1, "ello");
		printf("[%s]\n", foo.c_str());
		foo.replace(1, 1, "a");
		printf("[%s]\n", foo.c_str());
		foo.replace(6, 6, "");
		printf("[%s]\n", foo.c_str());
		foo.insert(5, " there");
		printf("[%s]\n", foo.c_str());
		foo.replace(1, 4, "abcdefghijkl", 8, 2);
		printf("[%s]\n", foo.c_str());
		foo.replace(1, 2, "hello there", 1, 4);
		printf("[%s]\n", foo.c_str());
		std::string bar = "hi greetings hello";
		foo.replace(foo.begin() + 6, foo.begin() + 11, bar.begin() + 3, bar.end() - 6);
		printf("[%s]\n", foo.c_str());
		foo.replace(0, 2, 10, '?');
		printf("[%s]\n", foo.c_str());
		foo.replace(0, 10, 3, 'h');
		printf("[%s]\n", foo.c_str());


		// timer_max = 10;
		// timer_addr = +[]() { printf("Timer done!\n"); };

		// for (size_t address = (size_t) multiboot_data;; address *= 1.1) {
		// 	Terminal::clear();
		// 	printf("Address: %ld -> %d", address, *((int *) address));
		// 	for (int j = 0; j < 30000000; ++j);
		// }

		for (;;);
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

					// If the kernel is in the memory space given by multiboot, increase memoryLow to just past it.
					if (memoryLow <= (uintptr_t) this && (uintptr_t) this < memoryHigh)
						memoryLow = Util::upalign(((uintptr_t) this) + sizeof(Kernel), 4096);

					if (memoryLow <= (uintptr_t) &high_page_directory_table && (uintptr_t) &high_page_directory_table < memoryHigh)
						memoryLow = Util::upalign(((uintptr_t) &high_page_directory_table) + PAGE_DIRECTORY_SIZE * PAGE_DIRECTORY_ENTRY_SIZE, 4096);

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
						// uint64_t addr_high = (uint64_t) mmap->addr_high << 32;
						// uint64_t len_high = (uint64_t) mmap->len_high << 32;
						// printf(" base_addr = 0x%lx, length = 0x%lx, type = %u\n", (uint64_t) mmap->addr_low | addr_high, (uint64_t) mmap->len_low | len_high, mmap->type);
					}
					break;
				}
#endif
			}
		}
	}

	void Kernel::arrangeMemory() {
		pageDescriptors = (char *) memoryLow;
		pagesLength = Util::downalign((memoryHigh - memoryLow) * 4096 / 4097, 4096);
		pagesStart = (void *) Util::downalign((uintptr_t) ((char *) memoryHigh - pagesLength), 4096);
		pageDescriptorsLength = (uintptr_t) pagesStart - memoryLow;
	}

	void Kernel::initPageDescriptors() {
		memset(pageDescriptors, 0, pageDescriptorsLength);
	}

	void Kernel::wait(size_t millimoments) {
		for (size_t i = 0; i < millimoments; ++i)
			for (size_t j = 0; j < 8000000; ++j);
	}
}
