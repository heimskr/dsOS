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
#include "DsUtil.h"
#include "device/IDEDevice.h"
#include "fs/dsFAT/dsFAT.h"
#include "fs/Partition.h"
#include "hardware/AHCI.h"
#include "hardware/IDE.h"
#include "hardware/MBR.h"
#include "hardware/PCI.h"
#include "hardware/PS2Keyboard.h"
#include "hardware/SATA.h"
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
		x86_64::IDT::init();
		initPageDescriptors();

		// printf("CR0: %x, CR2: %x, CR3: %x, CR4: %x\n", x86_64::getCR0(), x86_64::getCR2(), x86_64::getCR3(), x86_64::getCR4());
		// char model[13];
		// x86_64::getModel(model);
		// printf("Model: %s\n", model);
		// printf("APIC: %s\n", x86_64::checkAPIC()? "yes" : "no");
		printf("Memory: 0x%lx through 0x%lx\n", memoryLow, memoryHigh);
		// printf("Core count: %d\n", x86_64::coreCount());
		// printf("ARAT: %s\n", x86_64::arat()? "true" : "false");

		// These three lines are incredibly janky. Fixing them is important.
		uintptr_t bitmap_base = 0xa00000UL;
		uintptr_t physical_start = (bitmap_base + 1'000'000UL) & ~0xfff; // 1 MB is enough to map over 30 GB.
		pager = x86_64::PageMeta4K((void *) physical_start, (void *) 0xffff80800000UL, (void *) bitmap_base, (memoryHigh - physical_start) / 4096);

		pager.assignSelf();
		printf("[%s:%d]\n", __FILE__, __LINE__);
		pager.clear();
		printf("[%s:%d]\n", __FILE__, __LINE__);

		memory.setBounds((char *) 0xfffff00000000000UL, (char *) 0xfffffffffffff000UL);

		x86_64::APIC::init(*this);

		x86_64::PIC::clearIRQ(1);
		x86_64::PIC::clearIRQ(14);
		x86_64::PIC::clearIRQ(15);

		timer_addr = &::schedule;
		timer_max = 4;

		// printf("map size: %lu\n", map.size());

		x86_64::APIC::initTimer(2);
		x86_64::APIC::disableTimer();

		IDE::init();

		// PCI::printDevices();

		PCI::findAHCIController();

		if (PCI::Device *controller = AHCI::controller) {
			printf("Found AHCI controller.\n");
			printf("Command: %x / %b\n", controller->nativeHeader.command, controller->nativeHeader.command);
			PCI::HeaderNative &header = controller->nativeHeader;
			header.command &= ~PCI::COMMAND_INT_DISABLE;
			header.command |= PCI::COMMAND_MEMORY;
			PCI::writeWord(controller->bsf, PCI::COMMAND, header.command);

			volatile AHCI::HBAMemory *abar = (AHCI::HBAMemory *) (uint64_t) (controller->nativeHeader.bar5 & ~0xfff);
			printf("abar: 0x%lx\n", abar);
			pager.identityMap(abar, MMU_CACHE_DISABLED);
			pager.identityMap((char *) abar + 0x1000, MMU_CACHE_DISABLED);

			abar->probe();
			abar->cap = abar->cap | (1 << 31);
			abar->ghc = abar->ghc | (1 << 31);
			printf("Interrupt line: %d\n", controller->nativeHeader.interruptLine);
			printf("Interrupt pin:  %d\n", controller->nativeHeader.interruptPin);
			printf("cap: %u\n", abar->cap);
			printf("ghc: %u\n", abar->ghc);
			printf("is: %u\n", abar->is);
			printf("pi: %u\n", abar->pi);
			printf("vs: %u\n", abar->vs);
			printf("ccc_ctl: %u\n", abar->ccc_ctl);
			printf("ccc_pts: %u\n", abar->ccc_pts);
			printf("em_loc: %u\n", abar->em_loc);
			printf("em_ctl: %u\n", abar->em_ctl);
			printf("cap2: %u\n", abar->cap2);
			printf("bohc: %u\n", abar->bohc);
			for (int i = 0; i < 32; ++i) {
				volatile AHCI::HBAPort &port = abar->ports[i];
				if (port.clb == 0)
					continue;
				printf("--------------------------------\n");
				printf("Type: %s\n", AHCI::deviceTypes[(int) port.identifyDevice()]);
				printf("%d: clb: %x / %b\n", i, port.clb, port.clb);
				printf("%d: clbu: %x / %b\n", i, port.clbu, port.clbu);
				printf("%d: fb: %u / %b\n", i, port.fb, port.fb);
				printf("%d: fbu: %u / %b\n", i, port.fbu, port.fbu);
				printf("%d: is: %u / %b\n", i, port.is, port.is);
				printf("%d: ie: %u / %b\n", i, port.ie, port.ie);
				printf("%d: cmd: %u / %b\n", i, port.cmd, port.cmd);
				printf("%d: rsv0: %u / %b\n", i, port.rsv0, port.rsv0);
				printf("%d: tfd: %u / %b\n", i, port.tfd, port.tfd);
				printf("%d: sig: %u / %b\n", i, port.sig, port.sig);
				printf("%d: ssts: %u / %b\n", i, port.ssts, port.ssts);
				printf("%d: sctl: %u / %b\n", i, port.sctl, port.sctl);
				printf("%d: serr: %u / %b\n", i, port.serr, port.serr);
				printf("%d: sact: %u / %b\n", i, port.sact, port.sact);
				printf("%d: ci: %u / %b\n", i, port.ci, port.ci);
				printf("%d: sntf: %u / %b\n", i, port.sntf, port.sntf);
				printf("%d: fbs: %u / %b\n", i, port.fbs, port.fbs);
			}

			for (int portID = 0; portID <= 0; ++portID) {
				volatile AHCI::HBAPort &port = abar->ports[portID];
				char buffer[513] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
				printf("Port %d:\n", portID);
				printf("Result: %d\n", SATA::issueCommand(port, ATA::Command::IdentifyDevice, false, buffer, 1, 512, 0, 0));
				// printf("Result: %d\n", SATA::read(port, 0, 1, buffer));
				// printf("Result: %d\n", SATA::issueCommand(port, ATA::Command::ReadSectors, false, buffer, 4, 512, 0, 1));
				for (int i = 0; i < 512; ++i)
					printf("%c", buffer[i]);
				printf("\n");
			}
		} else {
			printf("No AHCI controller found.\n");
		}




		perish();

		MBR mbr;
		mbr.firstEntry = {1 << 7, 0x42, 1, 2047};
		IDE::writeBytes(0, sizeof(MBR), 0, &mbr);
		Device::IDEDevice device(0);
		FS::Partition first_partition(&device, IDE::SECTOR_SIZE, 2047 * IDE::SECTOR_SIZE);
		using namespace FS::DsFAT;
		auto driver = std::make_unique<DsFATDriver>(&first_partition);
		driver->make(320 * 5);
		int status;

		printf("\e[32;1;4mFirst readdir.\e[0m\n");
		status = driver->readdir("/", [](const char *path, off_t offset) { printf("\"%s\" @ %ld\n", path, offset); });
		if (status != 0) printf("readdir failed: %s\n", strerror(-status));

		printf("\e[32;1;4mCreating foo.\e[0m\n");
		status = driver->create("/foo", 0666);
		if (status != 0) printf("create failed: %s\n", strerror(-status));

		printf("\e[32;1;4mReaddir after creating foo.\e[0m\n");
		status = driver->readdir("/", [](const char *path, off_t offset) { printf("\"%s\" @ %ld\n", path, offset); });
		if (status != 0) printf("readdir failed: %s\n", strerror(-status));

		printf("\e[32;1;4mCreating bar.\e[0m\n");
		status = driver->create("/bar", 0666);
		if (status != 0) printf("create failed: %s\n", strerror(-status));

		printf("\e[32;1;4mReaddir after creating bar.\e[0m\n");
		status = driver->readdir("/", [](const char *path, off_t offset) { printf("\"%s\" @ %ld\n", path, offset); });
		if (status != 0) printf("readdir failed: %s\n", strerror(-status));

		printf("\e[32;1;4mDone.\e[0m\n");
		driver->superblock.print();

		// for (size_t i = 0; i < 50; ++i) {
		// 	printf("[%lu] %d\n", i, driver->readFAT(i));
		// }

		printf("offsetof(  DirEntry::      name) = 0x%lx = %lu\n", offsetof(DirEntry, name), offsetof(DirEntry, name));
		printf("offsetof(  DirEntry::     times) = 0x%lx = %lu\n", offsetof(DirEntry, times), offsetof(DirEntry, times));
		printf("offsetof(  DirEntry::    length) = 0x%lx = %lu\n", offsetof(DirEntry, length), offsetof(DirEntry, length));
		printf("offsetof(  DirEntry::startBlock) = 0x%lx = %lu\n", offsetof(DirEntry, startBlock), offsetof(DirEntry, startBlock));
		printf("offsetof(  DirEntry::      type) = 0x%lx = %lu\n", offsetof(DirEntry, type), offsetof(DirEntry, type));
		printf("offsetof(  DirEntry::     modes) = 0x%lx = %lu\n", offsetof(DirEntry, modes), offsetof(DirEntry, modes));
		printf("offsetof(  DirEntry::   padding) = 0x%lx = %lu\n", offsetof(DirEntry, padding), offsetof(DirEntry, padding));

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

	void Kernel::perish() {
		for (;;)
			asm("hlt");
	}

	x86_64::PageMeta4K & Kernel::getPager() {
		if (!instance) {
			printf("Kernel instance is null!\n");
			for (;;) asm("hlt");
		}

		return instance->pager;
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
