#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "DsUtil.h"
#include "Kernel.h"

namespace x86_64 {
	PageMeta::PageMeta(void *physical_start, void *virtual_start):
		physicalStart(physical_start), virtualStart(virtual_start) {}

	void * PageMeta::allocateFreePhysicalAddress() {
		int free_index = findFree();
		if (free_index == -1)
			return nullptr;
		mark(free_index, true);
		return (void *) ((uintptr_t) physicalStart + free_index * pageSize());
	}

	bool PageMeta::assignAddress(void *virtual_address, void *physical_address) {
		using PTW = PageTableWrapper;
		return assign(PTW::getPML4Index(virtual_address), PTW::getPDPTIndex(virtual_address),
		              PTW::getPDTIndex(virtual_address), PTW::getPTIndex(virtual_address), physical_address);
	}

	bool PageMeta::identityMap(void *address) {
		return assignAddress(address, address);
	}

	bool PageMeta::identityMap(volatile void *address) {
		return identityMap((void *) address);
	}

	uint64_t PageMeta::addressToEntry(void *address) const {
		return (((uint64_t) address) & ~0xfff) | MMU_PRESENT | MMU_WRITABLE;
	}

	PageMeta4K::PageMeta4K(): PageMeta(nullptr, nullptr), pages(-1) {}
	
	PageMeta4K::PageMeta4K(void *physical_start, void *virtual_start, void *bitmap_address, int pages_):
	PageMeta(physical_start, virtual_start), pages(pages_) {
		bitmap = new (bitmap_address) bitmap_t[DsOS::Util::updiv(pages_, 8 * (int) sizeof(bitmap_t))];
	}

	size_t PageMeta4K::bitmapSize() const {
		if (pages == -1 || !bitmap)
			return 0;
		return DsOS::Util::updiv((size_t) pages, 8 * sizeof(bitmap_t)) * sizeof(bitmap_t);
	}

	size_t PageMeta4K::pageCount() const {
		return pages;
	}

	size_t PageMeta4K::pageSize() const {
		return 4 << 10;
	}

	void PageMeta4K::clear() {
		if (pages == -1)
			return;
		memset(bitmap, 0, DsOS::Util::updiv(pages, 8 * (int) sizeof(bitmap_t)) * sizeof(bitmap_t));
	}

	int PageMeta4K::findFree() const {
		if (pages != -1)
			for (unsigned int i = 0; i < pages / (8 * sizeof(bitmap_t)); ++i)
				if (bitmap[i] != -1L)
					for (unsigned int j = 0; j < 8 * sizeof(bitmap_t); ++j)
						if ((bitmap[i] & (1 << j)) == 0)
							return (i * sizeof(bitmap_t)) + j;
		return -1;
	}

	void PageMeta4K::mark(int index, bool used) {
		if (pages == -1)
			return;
		if (used)
			bitmap[index / (8 * sizeof(bitmap_t))] |=   1 << (index % (8 * sizeof(bitmap_t)));
		else
			bitmap[index / (8 * sizeof(bitmap_t))] &= ~(1 << (index % (8 * sizeof(bitmap_t))));
	}

	uintptr_t PageMeta4K::assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
	                             void *physical_address) {
		if (pages == -1) {
			printf("pages == -1\n");
			return 0;
		}

		DsOS::Kernel *kernel = DsOS::Kernel::instance;
		if (!kernel) {
			printf("Kernel instance is null!\n");
			for (;;);
		}

		PageTableWrapper &wrapper = kernel->kernelPML4;
		if (wrapper.entries[pml4_index] == 0) {
			// Allocate a page for a new PDPT if the PML4E is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				wrapper.entries[pml4_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		}

		uint64_t *pdpt = (uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (pdpt[pdpt_index] == 0) {
			// Allocate a page for a new PDT if the PDPE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		}

		uint64_t *pdt = (uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (pdt[pdt_index] == 0) {
			// Allocate a page for a new PT if the PDE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pdt[pdt_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		}

		uint64_t *pt = (uint64_t *) (pdt[pdt_index] & ~0xfff);
		uintptr_t assigned = 0;
		if (pt[pt_index] == 0) {
			// Allocate a new page if the PTE is empty (or, optionally, use a provided physical address).
			if (physical_address) {
				pt[pt_index] = addressToEntry(physical_address);
			} else if (void *free_addr = allocateFreePhysicalAddress()) {
				pt[pt_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
			assigned = pt[pt_index];
		} else {
			// Nothing really needed to be done anyway...
		}

		return assigned;
	}

	void PageMeta4K::assignSelf() {
		if (pages == -1)
			return;
		uint16_t pml4i, pdpti, pdti, pti;
		for (size_t i = 0, bsize = bitmapSize(), psize = pageSize(); i < bsize; i += psize) {
			void *address = bitmap + i;
			pml4i = PageTableWrapper::getPML4Index(address);
			pdpti = PageTableWrapper::getPDPTIndex(address);
			 pdti = PageTableWrapper:: getPDTIndex(address);
			  pti = PageTableWrapper::  getPTIndex(address);
			assign(pml4i, pdpti, pdti, pti);
		}
	}

	PageMeta4K::operator bool() const {
		return pages != -1;
	}

	size_t PageMeta4K::pagesUsed() const {
		size_t out = 0;
		size_t popcnt = 0;
		for (size_t i = 0; i < bitmapSize() / sizeof(bitmap_t); ++i) {
			asm("popcnt %1, %0" : "=r"(popcnt) : "r"(bitmap[i]));
			out += popcnt;
		}
		return out;
	}
}
