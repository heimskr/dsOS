#include "arch/x86_64/kernel.h"
#include "arch/x86_64/mmu.h"
#include "kernel_core.h"
#include "multiboot2.h"

using Bitmap = uint32_t;

extern volatile char _kernel_physical_start[];
extern volatile char _kernel_physical_end;
extern volatile Bitmap _bitmap_start[];
extern volatile Bitmap _bitmap_end;
extern volatile uint64_t pml4[];
extern volatile uint32_t multiboot_magic;
extern volatile uint64_t multiboot_data;
extern volatile uint64_t memory_low;
extern volatile uint64_t memory_high;
extern volatile uint64_t physical_memory_map;

namespace Boot {
	constexpr static uint64_t pageSize = THORN_PAGE_SIZE;

	static inline int getPages() {
		return 8 * (&_bitmap_end - _bitmap_start);
	}

	static inline bool isPresent(uint64_t entry) {
		return entry & 1;
	}

	static inline uint16_t getPML4Index(uint64_t addr) { return (addr >> 39) & 0x1ff; }
	static inline uint16_t getPDPTIndex(uint64_t addr) { return (addr >> 30) & 0x1ff; }
	static inline uint16_t getPDTIndex (uint64_t addr) { return (addr >> 21) & 0x1ff; }
	static inline uint16_t getPTIndex  (uint64_t addr) { return (addr >> 12) & 0x1ff; }
	static inline uint16_t getOffset(uint64_t addr) { return addr & 0xfff; }

	static inline Bitmap * getBitmap() {
		return (Bitmap *) _bitmap_start;
	}

	int findFree(int start) {
		Bitmap *bitmap = getBitmap();
		const int pages = getPages();
		for (int i = start; i < pages / (8 * sizeof(Bitmap)); ++i)
			if (bitmap[i] != -1l)
				for (uint8_t j = 0; j < 8 * sizeof(Bitmap); ++j)
					if ((bitmap[i] & (1l << j)) == 0)
						return i * 8 * sizeof(Bitmap) + j;
		return -1;
	}

	static inline uint64_t addressToEntry(uint64_t address) {
		return (address & ~0xfff) | MMU_PRESENT | MMU_WRITABLE;
	}

	void markUsed(uint32_t index) {
		getBitmap()[index / (8 * sizeof(Bitmap))] |=   1l << (index % (8 * sizeof(Bitmap)));
	}

	void markFree(uint32_t index) {
		getBitmap()[index / (8 * sizeof(Bitmap))] &= ~(1l << (index % (8 * sizeof(Bitmap))));
	}

	bool isFree(uint32_t index) {
		// NB: Change the math here if Bitmap changes in size.
		static_assert(sizeof(Bitmap) == 4);
		return (getBitmap()[index >> 5] & (1 << (index & 31))) != 0;
	}

	uint64_t allocateFreePhysicalAddress(uint32_t consecutive_count) {
		if (consecutive_count == 0)
			return 0;

		uint64_t free_start = (((uint64_t) &_kernel_physical_end) + 0xfff) & ~0xfff;

		if (consecutive_count == 1) {
			int free_index = findFree(1);
			if (free_index == -1)
				return 0;
			markUsed(free_index);
			return free_start + free_index * pageSize;
		}

		int index = -1;
		for (;;) {
			index = findFree(index + 1);
			for (uint32_t i = 1; i < consecutive_count; ++i)
				if (!isFree(index + i))
					goto nope; // sorry
			markUsed(index);
			return free_start + index * pageSize;
			nope:
			continue;
		}
	}

	void allocate2MiB(uint64_t virt, uint64_t phys) {
		const auto pml4_index = getPML4Index(virt);
		const auto pdpt_index = getPDPTIndex(virt);
		const auto pdt_index  = getPDTIndex(virt);

		if (!isPresent(pml4[pml4_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pml4[pml4_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdpt = (volatile uint64_t *) (pml4[pml4_index] & ~0xfff);

		if (!isPresent(pdpt[pdpt_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdt = (volatile uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		pdt[pdt_index] = addressToEntry(phys) | MMU_PDE_TWO_MB;
	}

	void allocate4KiB(uint64_t virt, uint64_t phys) {
		const auto pml4_index = getPML4Index(virt);
		const auto pdpt_index = getPDPTIndex(virt);
		const auto pdt_index  = getPDTIndex(virt);
		const auto pt_index   = getPTIndex(virt);

		if (!isPresent(pml4[pml4_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pml4[pml4_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdpt = (volatile uint64_t *) (pml4[pml4_index] & ~0xfff);

		if (!isPresent(pdpt[pdpt_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdt = (volatile uint64_t *) (pdpt[pdpt_index] & ~0xfff);

		if (!isPresent(pdt[pdt_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pdt[pdt_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pt = (volatile uint64_t *) (pdt[pdt_index] & ~0xfff);
		pt[pt_index] = addressToEntry(phys);
	}

	static inline void allocate(uint64_t virt, uint64_t phys) {
		static_assert(pageSize == 4096 || pageSize == 2 << 20);

		if constexpr (pageSize == 4096) {
			allocate4KiB(virt, phys);
		} else {
			allocate2MiB(virt, phys);
		}
	}

	static inline void allocate(uint64_t virt) {
		allocate(virt, virt);
	}

	static void detectMemory() {
		struct multiboot_tag *tag;
		uint64_t addr = multiboot_data;

		if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
			for (;;) asm("hlt");
		}

		if (addr & 7) {
			for (;;) asm("hlt");
		}

		for (tag = (multiboot_tag *) (addr + 8);
		     tag->type != MULTIBOOT_TAG_TYPE_END;
		     tag = (multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))) {
			switch (tag->type) {
				case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
					memory_low  = ((multiboot_tag_basic_meminfo *) tag)->mem_lower * 1024;
					memory_high = ((multiboot_tag_basic_meminfo *) tag)->mem_upper * 1024;
					break;
			}
		}
	}

	extern "C" void setup_paging() {
		for (uint64_t page = 0; page < 512 * (pageSize == 4096? 512 : 1); ++page) {
			allocate(page * pageSize);
		}

		detectMemory();

		constexpr uint64_t stack_pages = 1 * (pageSize == 4096? 512 : 1);

		physical_memory_map = (0xfffffffffffff000ull - stack_pages * pageSize - memory_high) & ~0xfff;

		for (uint64_t i = 0; i <= memory_high; i += 4096)
			allocate(physical_memory_map + i, i);

		// Allocate space for kernel stack
		for (uint64_t page = 0; page < stack_pages; ++page) {
			allocate(0xfffffffffffff000 - page * pageSize, allocateFreePhysicalAddress(1));
		}
	}
}
