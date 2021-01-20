#pragma once

#include <functional>
#include <stddef.h>
#include <stdint.h>

#include "mmu.h"

namespace x86_64 {
	class PageMeta {
		public:
			void *physicalStart;
			void *virtualStart;
			virtual size_t pageCount() const = 0;
			virtual size_t pageSize() const = 0;
			virtual void clear() = 0;
			virtual int findFree() const = 0;
			virtual void * allocateFreePhysicalAddress();
			virtual void mark(int index, bool used) = 0;
			virtual uintptr_t assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
			                         volatile void *physical_address = nullptr, uint64_t extra_meta = 0) = 0;
			virtual size_t pagesUsed() const = 0;
			virtual operator bool() const = 0;
			virtual bool assignAddress(volatile void *virtual_address, volatile void *physical_address,
			                           uint64_t extra_meta = 0);
			virtual bool identityMap(volatile void *, uint64_t extra_meta = 0);
			/** Returns true if there was an entry for the given address. */
			virtual bool modifyEntry(volatile void *virtual_address, std::function<uint64_t(uint64_t)> modifier);
			/** Returns true if there was an entry for the given address. */
			virtual bool andMeta(volatile void *virtual_address, uint64_t meta);
			/** Returns true if there was an entry for the given address. */
			virtual bool orMeta(volatile void *virtual_address, uint64_t meta);
			virtual bool freeEntry(volatile void *virtual_address);

		protected:
			PageMeta(void *physical_start, void *virtual_start);
			uint64_t addressToEntry(volatile void *) const;
			inline bool isPresent(uint64_t entry) {
				return entry & MMU_PRESENT;
			}
	} __attribute__((packed));


	/** Number of 4KiB pages is PageCount + 1 to account for the other fields in PageMeta and PageMeta4K. */
	struct PageMeta4K: public PageMeta {
		using bitmap_t = int64_t;

		/** Number of pages. */
		int pages;

		/** Bits are 0 if the corresponding page is free, 1 if allocated. */
		bitmap_t *bitmap = nullptr;

		PageMeta4K();

		/** bitmap_address must be within a mapped page. */
		PageMeta4K(void *physical_start, void *virtual_start, void *bitmap_address, int pages_);

		/** Returns the size of the bitmap array in bytes. */
		size_t bitmapSize() const;
		virtual size_t pageCount() const override;
		virtual size_t pageSize() const override;
		virtual void clear() override;
		virtual int findFree() const override;
		virtual void mark(int index, bool used) override;
		virtual uintptr_t assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
		                         volatile void *physical_address = nullptr, uint64_t extra_meta = 0) override;
		/** Allocates pages for the bitmap array. */
		void assignSelf();
		virtual operator bool() const override;
		virtual size_t pagesUsed() const override;
	} __attribute__((packed));
}
