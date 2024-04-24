#pragma once

#include <functional>
#include <stddef.h>
#include <stdint.h>

#include "mmu.h"

extern volatile uint64_t physical_memory_map;
extern bool physical_memory_map_ready;

namespace x86_64 {
	class PageTableWrapper;

	class PageMeta {
		public:
			void *physicalStart = nullptr;
			uintptr_t physicalMemoryMap = 0;
			bool disableMemset = true;
			bool disablePresentCheck = false;
			bool physicalMemoryMapReady = false;
			virtual size_t pageCount() const = 0;
			virtual size_t pageSize() const = 0;
			virtual void clear() = 0;
			virtual int findFree(size_t start = 0) const = 0;
			virtual uintptr_t allocateFreePhysicalAddress(size_t consecutive_count = 1);
			uintptr_t allocateFreePhysicalFrame(size_t consecutive_count = 1);
			virtual void mark(int index, bool used) = 0;
			virtual uintptr_t assign(PageTableWrapper &, uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
			                         uintptr_t physical_address = 0, uint64_t extra_meta = 0) = 0;
			virtual size_t pagesUsed() const = 0;
			virtual bool isFree(size_t index) const = 0;
			virtual operator bool() const = 0;
			virtual bool assignAddress(PageTableWrapper &, uintptr_t virtual_address, uintptr_t physical_address = 0, uint64_t extra_meta = 0);
			virtual bool identityMap(PageTableWrapper &, uintptr_t , uint64_t extra_meta = 0);
			/** Returns true if there was an entry for the given address. */
			virtual bool modifyEntry(PageTableWrapper &, uintptr_t virtual_address, std::function<uint64_t(uint64_t)> modifier);
			/** Returns true if there was an entry for the given address. */
			virtual bool andMeta(PageTableWrapper &, uintptr_t virtual_address, uint64_t meta);
			/** Returns true if there was an entry for the given address. */
			virtual bool orMeta(PageTableWrapper &, uintptr_t virtual_address, uint64_t meta);
			virtual bool freeEntry(PageTableWrapper &, uintptr_t virtual_address);

		protected:
			explicit PageMeta(void *physical_start);
			uint64_t addressToEntry(volatile void *) const;
			uint64_t addressToEntry(uintptr_t) const;
			inline bool isPresent(uint64_t entry) {
				return entry & MMU_PRESENT;
			}

			virtual uintptr_t assignBeforePMM(PageTableWrapper &, uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index,
				uint16_t pt_index, uintptr_t physical_address = 0, uint64_t extra_meta = 0) = 0;
	};

	/** Number of 4KiB pages is PageCount + 1 to account for the other fields in PageMeta and PageMeta4K. */
	class PageMeta4K: public PageMeta {
		protected:
			virtual uintptr_t assignBeforePMM(PageTableWrapper &, uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
			                                  uintptr_t physical_address = 0, uint64_t extra_meta = 0) override;

		public:
			using Bitmap = int64_t;

			/** Number of pages. */
			int pages;

			/** Bits are 0 if the corresponding page is free, 1 if allocated. */
			Bitmap *bitmap = nullptr;

			PageMeta4K();

			/** bitmap_address must be within a mapped page. */
			PageMeta4K(void *physical_start, void *bitmap_address, int pages_);

			/** Returns the size of the bitmap array in bytes. */
			size_t bitmapSize() const;
			size_t pageCount() const override;
			size_t pageSize() const override;
			void clear() override;
			int findFree(size_t start = 0) const override;
			void mark(int index, bool used) override;
			uintptr_t assign(PageTableWrapper &, uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
			                 uintptr_t physical_address = 0, uint64_t extra_meta = 0) override;
			/** Allocates pages for the bitmap array. */
			void assignSelf(PageTableWrapper &);
			operator bool() const override;
			size_t pagesUsed() const override;
			bool isFree(size_t index) const override;
	};

	bool isKernelPMMReady();

	template <typename T>
	T & accessPhysical(uintptr_t addr) {
		if (!physical_memory_map_ready) {
			printf("Physical memory map not ready! (addr = 0x%lx)\n", addr);
			for (;;) asm("hlt");
		}

		return *reinterpret_cast<T *>(physical_memory_map + addr);
	}

	template <typename T>
	T & accessPhysical(void *addr) {
		return accessPhysical<T>(reinterpret_cast<uintptr_t>(addr));
	}

	template <typename T>
	const T & accessPhysical(const void *addr) {
		return accessPhysical<T>(reinterpret_cast<uintptr_t>(addr));
	}
}
