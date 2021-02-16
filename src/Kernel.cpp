#if defined(__linux__)
#warning "You are not using a cross-compiler. You will most certainly run into trouble."
#endif

#if !defined(__x86_64__)
#warning "The kernel needs to be compiled with an x86_64-elf compiler."
#endif

#include <memory>
#include <string>

#include "Kernel.h"
#include "Terminal.h"
#include "Test.h"
#include "ThornUtil.h"
#include "device/AHCIDevice.h"
#include "device/IDEDevice.h"
#include "fs/ThornFAT/ThornFAT.h"
#include "fs/Partition.h"
#include "hardware/AHCI.h"
#include "hardware/IDE.h"
#include "hardware/GPT.h"
#include "hardware/MBR.h"
#include "hardware/PCI.h"
#include "hardware/Ports.h"
#include "hardware/PS2Keyboard.h"
#include "hardware/SATA.h"
#include "hardware/Serial.h"
#include "hardware/UHCI.h"
#include "lib/globals.h"
#include "memory/memset.h"
#include "multiboot2.h"
#include "arch/x86_64/APIC.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/Interrupts.h"
#include "arch/x86_64/PIC.h"

extern void *multiboot_data;
extern unsigned int multiboot_magic;

extern void *tmp_stack;

#define DEBUG_MMAP

void schedule();

static bool waiting = true;

namespace Thorn {
	Kernel * Kernel::instance = nullptr;

	Kernel::Kernel(const x86_64::PageTableWrapper &pml4_): kernelPML4(pml4_) {
		if (Kernel::instance) {
			printf("Kernel instantiated twice!\n");
			for (;;);
		} else
			Kernel::instance = this;
	}

	void Kernel::wait(size_t num_ticks, uint32_t frequency) {
		waiting = true;
		ticks = 0;
		timer_max = num_ticks;
		timer_addr = +[] { waiting = false; };
		x86_64::APIC::initTimer(frequency);
		for (;;)
			if (waiting)
				asm("hlt");
			else {
				x86_64::APIC::disableTimer();
				break;
			}
	}

	void Kernel::main() {
		Terminal::clear();
		Terminal::write("Hello, kernel world!\n");
		if (Serial::init())
			Serial::write("\n\n\n");
		detectMemory();
		arrangeMemory();
		x86_64::IDT::init();
		initPageDescriptors();


		// These three lines are incredibly janky. Fixing them is important.
		uintptr_t bitmap_base = 0xa00000UL;
		uintptr_t physical_start = (bitmap_base + 5'000'000UL) & ~0xfff; // 5 MB is enough to map over 150 GB.
		pager = x86_64::PageMeta4K((void *) physical_start, (void *) 0xffff80800000UL, (void *) bitmap_base, (memoryHigh - physical_start) / 4096);

		pager.assignSelf();
		pager.clear();

		x86_64::APIC::init(*this);

		memory.setBounds((char *) 0xfffff00000000000UL, (char *) 0xfffffffffffff000UL);

		printf("Memory: 0x%lx through 0x%lx\n", memoryLow, memoryHigh);
		printf("Core count: %d\n", x86_64::coreCount());

		// Initialize global variables.
		unsigned ctor_count = ((uintptr_t) &_ctors_end - (uintptr_t) &_ctors_start) / sizeof(void *);
		for (unsigned i = 0; i < ctor_count; ++i)
			_ctors_start[i]();

		x86_64::PIC::clearIRQ(1);
		x86_64::PIC::clearIRQ(11);
		x86_64::PIC::clearIRQ(14);
		x86_64::PIC::clearIRQ(15);

		x86_64::APIC::initTimer(2);
		x86_64::APIC::disableTimer();

		// std::string str(10000, 'a');

		// UHCI::init();
		// IDE::init();
		PS2Keyboard::init();

		// PCI::printDevices();

		// PCI::scan();

		runTests();
		perish();
	}

	Kernel & Kernel::getInstance() {
		if (!instance) {
			printf("Kernel instance is null!\n");
			for (;;) asm("hlt");
		}

		return *instance;
	}

	void Kernel::backtrace() {
		uintptr_t *rbp;
		asm volatile("mov %%rbp, %0" : "=r"(rbp));
		printf("Backtrace (rbp = 0x%lx):\n", rbp);
		uintptr_t old = 0;
		for (int i = 0; (uintptr_t) rbp != 0; ++i) {
			if (old == *(rbp + 1))
				return;
			printf("[ 0x%lx ] (%d)\n", *(rbp + 1), i);
			if (i == 64) {
				printf("[ ... ]\n");
				return;
			}
			old = *(rbp + 1);
			rbp = (uintptr_t *) *rbp;
		}
	}

	void Kernel::backtrace(uintptr_t *rbp) {
		printf("Backtrace (rbp = 0x%lx):\n", rbp);
		uintptr_t old = 0;
		for (int i = 0; (uintptr_t) rbp != 0; ++i) {
			if (old == *(rbp + 1))
				return;
			printf("[ 0x%lx ] (%d)\n", *(rbp + 1), i);
			if (i == 64) {
				printf("[ ... ]\n");
				return;
			}
			old = *(rbp + 1);
			rbp = (uintptr_t *) *rbp;
		}
	}

	void Kernel::schedule() {
		ticks = 0;
		backtrace();
	}

	void Kernel::onKey(Keyboard::InputKey, bool) {
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

		for (tag = (multiboot_tag *) (addr + 8);
			tag->type != MULTIBOOT_TAG_TYPE_END;
			tag = (multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))) {

			switch (tag->type) {
				case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
					memoryLow  = ((multiboot_tag_basic_meminfo *) tag)->mem_lower * 1024;
					memoryHigh = ((multiboot_tag_basic_meminfo *) tag)->mem_upper * 1024;

					// If the kernel is in the memory space given by multiboot, increase memoryLow to just past it.
					if (memoryLow <= (uintptr_t) this && (uintptr_t) this < memoryHigh)
						memoryLow = Util::upalign(((uintptr_t) this) + sizeof(Kernel), 4096);

					if (memoryLow <= (uintptr_t) &high_page_directory_table && (uintptr_t) &high_page_directory_table < memoryHigh)
						memoryLow = Util::upalign(((uintptr_t) &high_page_directory_table) + PAGE_DIRECTORY_SIZE * PAGE_DIRECTORY_ENTRY_SIZE, 4096);

#ifdef DEBUG_MMAP
					printf("mem_lower = %uKB, mem_upper = %uKB\n",
						((multiboot_tag_basic_meminfo *) tag)->mem_lower,
						((multiboot_tag_basic_meminfo *) tag)->mem_upper);
#endif
					break;
#ifdef DEBUG_MMAP
				case MULTIBOOT_TAG_TYPE_MMAP: {
					multiboot_memory_map_t *mmap;
					// printf("mmap\n");
					for (mmap = ((multiboot_tag_mmap *) tag)->entries;
						(multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size;
						mmap = (multiboot_memory_map_t *)
								((uintptr_t) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size)) {
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

	void Kernel::perish() {
		for (;;)
			asm("hlt");
	}

	x86_64::PageMeta4K & Kernel::getPager() {
		if (!instance) {
			printf("Kernel instance is null!\n");
			perish();
		}

		return instance->pager;
	}
}

void schedule() {
	if (!Thorn::Kernel::instance) {
		printf("schedule(): Kernel instance is null!\n");
		Thorn::Kernel::perish();
	}
	Thorn::Kernel::instance->schedule();
}

extern "C" void kernel_main() {
	Thorn::Kernel kernel(x86_64::PageTableWrapper(&pml4, x86_64::PageTableWrapper::Type::PML4));
	kernel.main();
}
