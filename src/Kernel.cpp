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
			else
				break;
	}

	void Kernel::main() {
		Terminal::clear();
		Terminal::write("Hello, kernel world!\n");
		if (Serial::init())
			Serial::write("\n\n\n");
		// kernelPML4.print(false);
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
		// printf("[%s:%d]\n", __FILE__, __LINE__);
		pager.clear();
		// printf("[%s:%d]\n", __FILE__, __LINE__);

		AHCI::controllers = new std::vector<AHCI::Controller>();
		x86_64::APIC::init(*this);

		memory.setBounds((char *) 0xfffff00000000000UL, (char *) 0xfffffffffffff000UL);
		// wait(5);

		x86_64::PIC::clearIRQ(1);
		x86_64::PIC::clearIRQ(14);
		x86_64::PIC::clearIRQ(15);

		// timer_addr = &::schedule;
		// timer_max = 4;

		// printf("map size: %lu\n", map.size());

		x86_64::APIC::initTimer(2);
		x86_64::APIC::disableTimer();

		// IDE::init();

		PCI::printDevices();

		// PCI::scan();

		// printf("Scanned.\n\n");

		// perish();

		// wait(5);

		AHCI::Controller *last_controller = nullptr;

		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t device = 0; device < 32; ++device)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = PCI::getVendorID(bus, device, function);
					if (vendor == PCI::INVALID_VENDOR)
						continue;

					const uint32_t baseclass = PCI::getBaseClass(bus, device, function);
					const uint32_t subclass  = PCI::getSubclass(bus, device, function);
					if (baseclass != 1 || subclass != 6)
						continue;

					AHCI::controllers->push_back(AHCI::Controller(PCI::initDevice({bus, device, function})));
					AHCI::Controller &controller = AHCI::controllers->back();
					controller.init(*this);
					last_controller = &controller;
					volatile AHCI::HBAMemory *abar = controller.abar;
					printf("Controller at %x:%x:%x (abar = 0x%llx):", bus, device, function, abar);
					printf(" %dL.%dP ", controller.device->getInterruptLine(), controller.device->getInterruptPin());
					printf(" cap=%b", abar->cap);
					printf("\n");
					// wait(1);

					for (int i = 0; i < 32; ++i) {
						volatile AHCI::HBAPort &port = abar->ports[i];
						if (port.clb == 0)
							continue;

						controller.ports[i] = new AHCI::Port(&port, abar);
						controller.ports[i]->init();
						ATA::DeviceInfo info;
						controller.ports[i]->identify(info);
						// wait(5);

						char model[41];
						info.copyModel(model);
						printf("Model: \"%s\"\n", model);

						// if (controller.ports[i]->type != AHCI::DeviceType::Null) {
						// 	char buffer[513] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
						// 	printf("Result for port %d: %d\n", i, controller.ports[i]->read(0, 512, buffer));
						// 	for (int i = 0; i < 512; ++i)
						// 		printf("%c", buffer[i]);
						// 	printf("\n");
						// }
						printf("%d done.\n", i);
					}
					printf("\nNext.\n");
				}
		printf("Done.\n");

		if (last_controller) {
			MBR mbr;
			AHCI::Port *port = nullptr;
			for (int i = 0; i < 32; ++i)
				if (last_controller->ports[i]) {
					port = last_controller->ports[i];
					break;
				}
			if (port) {
				port->read(0, 512, &mbr);
				if (mbr.indicatesGPT()) {
					GPT::Header gpt;
					port->readBytes(sizeof(GPT::Header), AHCI::Port::BLOCKSIZE, &gpt);
					printf("Signature:   0x%lx\n", gpt.signature);
					printf("Revision:    %d\n", gpt.revision);
					printf("Header size: %d\n", gpt.headerSize);
					printf("Current LBA: %ld\n", gpt.currentLBA);
					printf("Other LBA:   %ld\n", gpt.otherLBA);
					printf("First LBA:   %ld\n", gpt.firstLBA);
					printf("Last LBA:    %ld\n", gpt.lastLBA);
					printf("Start LBA:   %ld\n", gpt.startLBA);
					printf("Partitions:  %d\n", gpt.partitionCount);
					printf("Entry size:  %d\n", gpt.partitionEntrySize);
					size_t offset = AHCI::Port::BLOCKSIZE * gpt.startLBA;
					gpt.guid.print(true);
					if (gpt.partitionEntrySize != sizeof(GPT::PartitionEntry)) {
						printf("Unsupported partition entry size.\n");
					} else {
						GPT::PartitionEntry first_entry;
						for (unsigned i = 0; i < gpt.partitionCount; ++i) {
							GPT::PartitionEntry entry;
							port->readBytes(gpt.partitionEntrySize, offset, &entry);
							if (entry.typeGUID) {
								printf("Partition %d: \"", i);
								entry.printName(false);
								printf("\", type GUID[%s], partition GUID[%s], first[%ld], last[%ld]\n",
									std::string(entry.typeGUID).c_str(), std::string(entry.partitionGUID).c_str(),
									entry.firstLBA, entry.lastLBA);
								if (!first_entry.typeGUID)
									first_entry = entry;
							}
							offset += gpt.partitionEntrySize;
						}

						if (first_entry.typeGUID) {
							printf("Using partition \"%s\".\n", std::string(first_entry).c_str());
							Device::AHCIDevice device(port);
							FS::Partition partition(&device, first_entry.firstLBA * AHCI::Port::BLOCKSIZE,
								(first_entry.lastLBA - first_entry.firstLBA + 1) * AHCI::Port::BLOCKSIZE);
							using namespace FS::ThornFAT;
							auto driver = std::make_unique<ThornFATDriver>(&partition);
							driver->make(sizeof(DirEntry) * 5);
							printf("Made a ThornFAT partition.\n");
							int status;

							printf("\e[32;1;4mFirst readdir.\e[0m\n");
							status = driver->readdir("/", [](const char *path, off_t offset) {
								printf("\"%s\" @ %ld\n", path, offset); });
							if (status != 0) printf("readdir failed: %s\n", strerror(-status));

							printf("\e[32;1;4mCreating foo.\e[0m\n");
							status = driver->create("/foo", 0666);
							if (status != 0) printf("create failed: %s\n", strerror(-status));

							printf("\e[32;1;4mReaddir after creating foo.\e[0m\n");
							status = driver->readdir("/", [](const char *path, off_t offset) {
								printf("\"%s\" @ %ld\n", path, offset); });
							if (status != 0) printf("readdir failed: %s\n", strerror(-status));

							printf("\e[32;1;4mCreating bar.\e[0m\n");
							status = driver->create("/bar", 0666);
							if (status != 0) printf("create failed: %s\n", strerror(-status));

							printf("\e[32;1;4mReaddir after creating bar.\e[0m\n");
							status = driver->readdir("/", [](const char *path, off_t offset) {
								printf("\"%s\" @ %ld\n", path, offset); });
							if (status != 0) printf("readdir failed: %s\n", strerror(-status));

							printf("\e[32;1;4mDone.\e[0m\n");
							driver->superblock.print();
						}
					}
				}
			} else printf(":[\n");
		} else printf(":(\n");

		perish();

/*
		MBR mbr;
		mbr.firstEntry = {1 << 7, 0x42, 1, 2047};
		IDE::writeBytes(0, sizeof(MBR), 0, &mbr);
		Device::IDEDevice device(0);
		FS::Partition first_partition(&device, IDE::SECTOR_SIZE, 2047 * IDE::SECTOR_SIZE);
		using namespace FS::ThornFAT;
		auto driver = std::make_unique<ThornFATDriver>(&first_partition);
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
//*/

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

	void Kernel::waitm(size_t millimoments) {
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
	if (!Thorn::Kernel::instance) {
		printf("schedule(): Kernel instance is null!\n");
		for (;;);
	}
	Thorn::Kernel::instance->schedule();
}

extern "C" void kernel_main() {
	Thorn::Kernel kernel(x86_64::PageTableWrapper(&pml4, x86_64::PageTableWrapper::Type::PML4));
	kernel.main();
}
