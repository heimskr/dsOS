#if defined(__linux__)
#warning "You are not using a cross-compiler. You will most certainly run into trouble."
#endif

#if !defined(__x86_64__)
#warning "The kernel needs to be compiled with an x86_64-elf compiler."
#endif

#include <string>

#include "Kernel.h"
#include "Terminal.h"
#include "DsUtil.h"
#include "device/IDEDevice.h"
#include "fs/dsFAT/dsFAT.h"
#include "fs/Partition.h"
#include "hardware/IDE.h"
#include "hardware/MBR.h"
#include "hardware/PS2Keyboard.h"
#include "hardware/Serial.h"
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
		// kernelPML4.print(false);

		Terminal::clear();
		Terminal::write("Hello, kernel World!\n");
		if (Serial::init())
			for (char ch: "\n\n\n")
				Serial::write(ch);
		detectMemory();
		arrangeMemory();
		initPageDescriptors();
		x86_64::IDT::init();

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

		x86_64::PIC::clearIRQ(1);
		x86_64::PIC::clearIRQ(14);

		timer_addr = &::schedule;
		timer_max = 4;

		// printf("map size: %lu\n", map.size());

		x86_64::APIC::initTimer(2);
		x86_64::APIC::disableTimer();


		IDE::init();

		MBR mbr;
		mbr.firstEntry = {1 << 7, 0x42, 1, 2047};
		IDE::writeBytes(0, sizeof(MBR), 0, &mbr);
		Device::IDEDevice device(0);
		FS::Partition first_partition(&device, IDE::SECTOR_SIZE, 2047 * IDE::SECTOR_SIZE);
		using namespace FS::DsFAT;
		DsFATDriver driver(&first_partition);
		driver.make(320 * 5);
		int status = driver.readdir("/", [](const char *path, off_t offset) {
			printf("\"%s\" @ %ld\n", path, offset);
		});
		if (status != 0)
			printf("readdir failed: %s\n", strerror(-status));

		driver.superblock.print();

		for (;;) {
			if (last_scancode == (0x2c | 0x80)) { // z
				last_scancode = 0;
				printf("Hello!\n");
				std::string str(50000, 'A');
				printf("[%s:%d]\n", __FILE__, __LINE__);
				printf("(");
				for (char &ch: str) {
					if (ch != 'A') {
						printf_putc = false;
						printf("[%d 0x%lx]\n", ch, &ch);
						printf_putc = true;
					} else
						printf("%c", ch);
				}
				printf(")\n");
				printf("[%s:%d]\n", __FILE__, __LINE__);
			} else if (last_scancode == (0x2b | 0x80)) { // backslash
				last_scancode = 0;
				char buffer[2048] = {0};
				printf(":: 0x%lx\n", &irqInvoked);

				printf_putc = false;
				for (int sector = 0; sector < 5; ++sector) {
					printf("(%d)\n", IDE::readSectors(1, 1, sector, buffer));
					for (size_t i = 0; i < sizeof(buffer); ++i)
						printf("%c", buffer[i]);
					printf("\n----------------------------\n");
					memset(buffer, 0, sizeof(buffer));
				}
				printf_putc = true;
				printf("\"%s\"\n", buffer);
			}
			asm volatile("hlt");
		}
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
		ticks = 0;
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

extern "C" void kernel_main() {
	DsOS::Kernel kernel(x86_64::PageTableWrapper(&pml4, x86_64::PageTableWrapper::Type::PML4));
	kernel.main();
}
