#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "ThornUtil.h"
#include "Kernel.h"

extern bool abouttodie;

namespace x86_64 {
	PageMeta::PageMeta(void *physical_start, void *virtual_start):
		physicalStart(physical_start), virtualStart(virtual_start) {}

	void * PageMeta::allocateFreePhysicalAddress(size_t consecutive_count) {
		if (consecutive_count == 0)
			return nullptr;

		if (consecutive_count == 1) {
			int free_index = findFree();
			if (free_index == -1)
				return nullptr;
			mark(free_index, true);
			return (void *) ((uintptr_t) physicalStart + free_index * pageSize());
		}

		int index = -1;
		for (;;) {
			index = findFree(index + 1);
			for (size_t i = 1; i < consecutive_count; ++i)
				if (!isFree(index + i))
					goto nope; // sorry
			mark(index, true);
			return (void *) ((uintptr_t) physicalStart + index * pageSize());
			nope:
			continue;
		}
	}

	bool PageMeta::assignAddress(volatile void *virtual_address, volatile void *physical_address, uint64_t extra_meta) {
		using PTW = PageTableWrapper;
		return assign(PTW::getPML4Index(virtual_address), PTW::getPDPTIndex(virtual_address),
		              PTW::getPDTIndex(virtual_address), PTW::getPTIndex(virtual_address),
		              physical_address, extra_meta);
	}

	bool PageMeta::identityMap(volatile void *address, uint64_t extra_meta) {
		return assignAddress(address, address, extra_meta);
	}

	bool PageMeta::modifyEntry(volatile void *virtual_address, std::function<uint64_t(uint64_t)> modifier) {
		Thorn::Kernel *kernel = Thorn::Kernel::instance;
		if (!kernel) {
			printf("Kernel instance is null!\n");
			for (;;) asm("hlt");
		}

		// printf("\e[31mModifying\e[0m 0x%lx\n", virtual_address);
		using PTW = PageTableWrapper;
		const uint16_t pml4_index = PTW::getPML4Index(virtual_address);
		const uint16_t pdpt_index = PTW::getPDPTIndex(virtual_address);
		const uint16_t pdt_index = PTW::getPDTIndex(virtual_address);
		const uint16_t pt_index = PTW::getPTIndex(virtual_address);
		PageTableWrapper &wrapper = kernel->kernelPML4;
		if (!isPresent(wrapper.entries[pml4_index]))
			return false;

		uint64_t *pdpt = (uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (!isPresent(pdpt[pdpt_index]))
			return false;

		uint64_t *pdt = (uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (!isPresent(pdt[pdt_index]))
			return false;

		uint64_t *pt = (uint64_t *) (pdt[pdt_index] & ~0xfff);
		if (!isPresent(pt[pt_index]))
			return false;

		pt[pt_index] = modifier(pt[pt_index]);
		return true;
	}

	bool PageMeta::andMeta(volatile void *virtual_address, uint64_t meta) {
		return modifyEntry(virtual_address, [meta](uint64_t entry) {
			return entry & meta;
		});
	}

	bool PageMeta::orMeta(volatile void *virtual_address, uint64_t meta) {
		return modifyEntry(virtual_address, [meta](uint64_t entry) {
			return entry | meta;
		});
	}

	bool PageMeta::freeEntry(volatile void *virtual_address) {
		// printf("\e[31mfreeEntry\e[0m 0x%lx\n", virtual_address);
		return modifyEntry(virtual_address, [](uint64_t) { return 0; });
	}

	uint64_t PageMeta::addressToEntry(volatile void *address) const {
		return (((uint64_t) address) & ~0xfff) | MMU_PRESENT | MMU_WRITABLE;
	}

	PageMeta4K::PageMeta4K(): PageMeta(nullptr, nullptr), pages(-1) {

	}
	
	PageMeta4K::PageMeta4K(void *physical_start, void *virtual_start, void *bitmap_address, int pages_):
	PageMeta(physical_start, virtual_start), pages(pages_) {
		const size_t bitmap_count = Thorn::Util::updiv(pages_, 8 * (int) sizeof(bitmap_t));
		bitmap = new (bitmap_address) bitmap_t[bitmap_count];
		const size_t page_count = Thorn::Util::updiv(bitmap_count * sizeof(bitmap_t), 4096UL);
		for (size_t i = 0; i < page_count; ++i)
			assignAddress(static_cast<char *>(bitmap_address) + 4096 * i);
	}

	size_t PageMeta4K::bitmapSize() const {
		if (pages == -1 || !bitmap)
			return 0;
		return Thorn::Util::updiv((size_t) pages, 8 * sizeof(bitmap_t)) * sizeof(bitmap_t);
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
		memset(bitmap, 0, Thorn::Util::updiv(pages, 8 * (int) sizeof(bitmap_t)) * sizeof(bitmap_t));
	}

	int PageMeta4K::findFree(size_t start) const {
		if (pages != -1)
			for (size_t i = start; i < pages / (8 * sizeof(bitmap_t)); ++i)
				if (bitmap[i] != -1L)
					for (unsigned int j = 0; j < 8 * sizeof(bitmap_t); ++j)
						if ((bitmap[i] & (1L << j)) == 0)
							return i * 8 * sizeof(bitmap_t) + j;
		return -1;
	}

	void PageMeta4K::mark(int index, bool used) {
		if (pages == -1) {
			printf("[PageMeta4K::mark] pages == -1\n");
			return;
		}

		if (used)
			bitmap[index / (8 * sizeof(bitmap_t))] |=   1L << (index % (8 * sizeof(bitmap_t)));
		else
			bitmap[index / (8 * sizeof(bitmap_t))] &= ~(1L << (index % (8 * sizeof(bitmap_t))));
	}

	uintptr_t PageMeta4K::assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
	                             volatile void *physical_address, uint64_t extra_meta) {
		// serprintf("\e[32massign\e[0m %u, %u, %u, %u, 0x%lx, 0x%lx\n", pml4_index, pdpt_index, pdt_index, pt_index, physical_address, extra_meta);

		if (pages == -1) {
			printf("[PageMeta4K::assign] pages == -1\n");
			return 0;
		}

		Thorn::Kernel *kernel = Thorn::Kernel::instance;
		if (!kernel) {
			printf("Kernel instance is null!\n");
			for (;;) asm("hlt");
		}

		PageTableWrapper &wrapper = kernel->kernelPML4;
		if (!Thorn::Util::isCanonical(wrapper.entries)) {
			printf("PML4 (0x%lx) isn't canonical!\n", wrapper.entries);
			for (;;) asm("hlt");
		}
		if (!isPresent(wrapper.entries[pml4_index])) {
			// Allocate a page for a new PDPT if the PML4E is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				wrapper.entries[pml4_index] = addressToEntry(free_addr);
				memset(free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		uint64_t *pdpt = (uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (!Thorn::Util::isCanonical(pdpt)) {
			kernel->kernelPML4.print(false);
			printf("PDPT (0x%lx) isn't canonical!\n", pdpt);
			for (;;) asm("hlt");
		}
		if (!isPresent(pdpt[pdpt_index])) {
			// Allocate a page for a new PDT if the PDPE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				memset(free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		uint64_t *pdt = (uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (!Thorn::Util::isCanonical(pdt)) {
			kernel->kernelPML4.print(false);
			printf("PDT (0x%lx) isn't canonical!\n", pdt);
			for (;;) asm("hlt");
		}
		if (!isPresent(pdt[pdt_index])) {
			// Allocate a page for a new PT if the PDE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pdt[pdt_index] = addressToEntry(free_addr);
				memset(free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		uint64_t *pt = (uint64_t *) (pdt[pdt_index] & ~0xfff);
		uintptr_t assigned = 0;
		if (!Thorn::Util::isCanonical(pt)) {
			kernel->kernelPML4.print(false);
			printf("PT (0x%lx) isn't canonical!\n", pt);
			for (;;) asm("hlt");
		}

		if (!isPresent(pt[pt_index])) {
			// Allocate a new page if the PTE is empty (or, optionally, use a provided physical address).
			if (physical_address) {
				pt[pt_index] = addressToEntry(physical_address);
			} else if (void *free_addr = allocateFreePhysicalAddress()) {
				pt[pt_index] = addressToEntry(free_addr) | extra_meta;
				memset(free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
			assigned = pt[pt_index];
		} else {
			// Nothing really needed to be done anyway...
		}

		return assigned;
	}

	void PageMeta4K::assignSelf() {
		if (pages == -1) {
			printf("PageMeta4K::assignSelf failed: pages == -1\n");
			return;
		}

		uint16_t pml4i, pdpti, pdti, pti;
		const size_t bsize = bitmapSize(), psize = pageSize();
		for (size_t i = 0; i < bsize; i += psize) {
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

	bool PageMeta4K::isFree(size_t index) const {
		// NB: Change the math here if bitmap_t changes in size.
		return (bitmap[index >> 6] & (1 << (index & 63))) != 0;
	}
}
