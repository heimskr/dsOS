#if defined(__linux__)
#warning "You are not using a cross compiler. You will most certainly run into trouble."
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

extern volatile uint32_t multiboot_magic;
extern volatile uint64_t multiboot_data;
extern volatile uint64_t _bitmap_start[];
extern volatile uint64_t memory_low;
extern volatile uint64_t memory_high;
extern volatile uint64_t physical_memory_map;
extern volatile char _kernel_physical_end;
bool physical_memory_map_ready = false;

#define DEBUG_MMAP

void schedule();

#define XSTR(x) #x
#define HELLO_(x) Thorn::Serial::write(__FILE__ ":" XSTR(x) "\n")
#define HELLO HELLO_(__LINE__)

namespace Thorn {
	Kernel * Kernel::instance = nullptr;

	Kernel::Kernel(const x86_64::PageTableWrapper &pml4_): kernelPML4(pml4_) {
		if (Kernel::instance) {
			printf("Kernel instantiated twice!\n");
			for (;;) asm("hlt");
		} else
			Kernel::instance = this;
	}

	void Kernel::wait(size_t num_ticks, uint32_t frequency) {
		x86_64::APIC::wait(num_ticks, frequency);
	}

	void Kernel::main() {
		Terminal::clear();
		Terminal::write("Hello, world!\n");
		if (Serial::init())
			Serial::write("\n\n\n");
		x86_64::IDT::init();
		detectMemory();
		HELLO;
		arrangeMemory();
		physical_memory_map_ready = true;
		HELLO;
		initPageDescriptors();
		HELLO;

		const uintptr_t bitmap_base  = (uintptr_t) &_bitmap_start;
		const uintptr_t physical_end = (uintptr_t) &_kernel_physical_end;

		{
			Lock<Mutex> lock;
			auto &pager = lockedPager.get(lock);
			printf("Initializing pager. pml4 is at 0x%lx.\n", kernelPML4.entries);
			pager = x86_64::PageMeta4K((void *) physical_end, (void *) bitmap_base, (memory_high - physical_end) / 4096);

			printf("physical_end = 0x%lx\n", physical_end);
		}

		// kernelPML4.print(false, false);

		initPhysicalMemoryMap();

		x86_64::APIC::init(*this);

		memory.setBounds((char *) 0xfffff00000000000ul, (char *) Util::downalign((uintptr_t) physicalMemoryMap - 4096, 4096));

		printf("Memory: 0x%lx through 0x%lx\n", memory_low, memory_high);
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
		// x86_64::APIC::disableTimer();

		// std::string str(10000, 'a');

		// UHCI::init();
		// IDE::init();

		initPointers();

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

	void Kernel::onKey(Keyboard::InputKey /* key */, bool /* down */) {
		// if (!down && key == Keyboard::InputKey::KeyC && Keyboard::isModifier(Keyboard::Modifier::Ctrl))
			// looping = false;
	}

	void Kernel::initPointers() {
		processes = std::make_unique<decltype(processes)::element_type>();
		pageFrameProcesses = std::make_unique<decltype(pageFrameProcesses)::element_type>();
	}

	Process & Kernel::makeProcess() {
		Lock<RecursiveMutex> lock;
		auto &procs = processes->get(lock);

		PID pid = nextPID();
		assert(!procs.contains(pid));
		Process &out = procs[pid];
		out.init();
		return out;
	}

	void Kernel::detectMemory() {
		struct multiboot_tag *tag;
		uintptr_t addr = (uintptr_t) multiboot_data;

		if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
			printf("Invalid magic number: %d\n", (unsigned) multiboot_magic);
			return;
		}

		if (addr & 7) {
			printf("Unaligned mbi: %d\n", addr);
			return;
		}

		printf("\e[32mtag = 0x%lx\e[39m\n", addr + 8);

		// If the kernel is in the memory space given by multiboot, increase memory_low to just past it.
		if (memory_low <= (uintptr_t) this && (uintptr_t) this < memory_high)
			memory_low = Util::upalign(((uintptr_t) this) + sizeof(Kernel), 4096);

		for (tag = (multiboot_tag *) (addr + 8);
		     tag->type != MULTIBOOT_TAG_TYPE_END;
		     tag = (multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))) {

			printf("\e[33mtag = 0x%lx\e[39m\n", tag);

			switch (tag->type) {
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
		pageDescriptors = (char *) memory_low;
		pagesLength = Util::downalign((memory_high - memory_low) * 4096 / 4097, 4096);
		pagesStart = (void *) Util::downalign((uintptr_t) ((char *) memory_high - pagesLength), 4096);
		pageDescriptorsLength = (uintptr_t) pagesStart - memory_low;
	}

	void Kernel::initPhysicalMemoryMap() {
		printf("memory_low  = 0x%lx\n", memory_low);
		printf("memory_high = 0x%lx\n", memory_high);
		// physicalMemoryMap = (void *) Util::downalign((0xfffffffffffff000ul - 1 * 512 * 4096 - memory_high), 4096);
		physicalMemoryMap = physical_memory_map;
		printf("physicalMemoryMap = 0x%lx\n", physicalMemoryMap);
		printf("Mapping all physical memory...\n");
		Lock<Mutex> pager_lock;
		auto &pager = getPager(pager_lock);
		const bool old_disable_memset = pager.disableMemset;
		const bool old_disable_present_check = pager.disablePresentCheck;
		pager.disableMemset = true;
		pager.disablePresentCheck = false;
		pager.physicalMemoryMap = physicalMemoryMap;
		// for (uintptr_t i = 0; i <= memory_high; i += 4096)
		// 	pager.assignAddress(kernelPML4, (char *) physicalMemoryMap + i, (void *) i);
		pager.disableMemset = old_disable_memset;
		pager.disablePresentCheck = old_disable_present_check;
		pager.physicalMemoryMapReady = true;
		printf("...finished mapping physical memory.\n");
	}

	void Kernel::initPageDescriptors() {
		printf("pageDescriptors: 0x%lx\n", pageDescriptors);
		printf("pageDescriptorsLength: 0x%lx\n", pageDescriptorsLength);
		memset(pageDescriptors, 0, pageDescriptorsLength);
		HELLO;
	}

	PID Kernel::nextPID() {
		if (lastPID >= MaxPID) {
			lastPID = 1;
		}

		Lock<RecursiveMutex> lock;
		auto &procs = processes->get(lock);

		if (procs.size() >= MaxPID - 5) {
			printf("Too many processes!\n");
			perish();
		}

		PID candidate = lastPID + 1;

		auto iter = procs.find(candidate);

		for (;;) {
			if (iter == procs.end())
				return lastPID = candidate;

			if (++iter == procs.end())
				return lastPID = candidate + 1;

			auto next = iter->first;
			if (++candidate < next)
				return lastPID = candidate;

			iter = procs.find(candidate);
		}
	}

	void Kernel::perish() {
		for (;;)
			asm("hlt");
	}

	x86_64::PageMeta4K & Kernel::getPager(Lock<Mutex> &lock) {
		verifyInstance();
		return instance->lockedPager.get(lock);
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
	Thorn::Kernel kernel(x86_64::PageTableWrapper(pml4, x86_64::PageTableWrapper::Type::PML4));
	kernel.main();
}
