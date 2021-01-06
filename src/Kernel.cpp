#include "Kernel.h"
#include "Terminal.h"
#include "DsUtil.h"
#include "hardware/Serial.h"
#include "memory/memset.h"
#include "multiboot2.h"
#include "arch/x86_64/APIC.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/Interrupts.h"
#include "arch/x86_64/PIC.h"
#include <string>
#include <unordered_map>

extern void *multiboot_data;
extern unsigned int multiboot_magic;

extern void *tmp_stack;

#define DEBUG_MMAP

void schedule();

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
		detectMemory();
		arrangeMemory();
		initPageDescriptors();
		x86_64::IDT::init();

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


		pager = x86_64::PageMeta4K((void *) 0x800000UL, (void *) 0xffff80800000UL, (void *) 0x600000UL, (memoryHigh - 0x800000UL) / 4096);
		pager.assignSelf();
		pager.clear();

		memory.setBounds((char *) 0xfffff00000000000UL, (char *) 0xfffffffffffff000UL);

		x86_64::APIC::init(*this);

		kernelPML4.print(false);

		x86_64::PIC::clearIRQ(1);

		timer_addr = &::schedule;
		timer_max = 4;

		std::unordered_map<int, std::string> map;

		printf("%lu\n", map.size());

		// map[42] = std::string(0x1000 - 0x00, 'X');
		constexpr char special = 'Z';
		map[42] = std::string(0x2500, special);

		for (const std::pair<const int, std::string> &pair: map) {
			// printf("%d, 0x%lx, \"%s\"\n", pair.first, pair.second.c_str(), pair.second.c_str());
			const char *str = pair.second.c_str();
			printf("%d, 0x%lx, \"", pair.first, str);
			for (size_t i = 0; i < pair.second.size(); ++i) {
				printf("%c", str[i]);
				if (str[i] != special) {
					printf_putc = false;
					int x = str[i];
					printf("[%d: 0x%lx]\n", x & 0xff, &str[i]);
					printf_putc = true;
				}
			}
			printf("\"\n");
		}

		// x86_64::APIC::initTimer(1);
		// x86_64::APIC::disableTimer();

		// schedule();

		for (;;);
	}

	void Kernel::backtrace() {
		uint64_t *rbp;
		asm volatile("mov %%rbp, %0" : "=r"(rbp));
		printf("Backtrace:\n");
		for (int i = 0; (uintptr_t) rbp != 0; ++i) {
			printf("[ 0x%lx ] (%d)\n", *(rbp + 1), i);
			rbp = (uint64_t *) *rbp;
		}
	}

	void Kernel::schedule() {
		backtrace();
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

void schedule() {
	if (!DsOS::Kernel::instance) {
		printf("schedule(): Kernel instance is null!\n");
		for (;;);
	}
	DsOS::Kernel::instance->schedule();
}
