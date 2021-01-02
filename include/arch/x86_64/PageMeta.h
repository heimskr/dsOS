#pragma once

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
			virtual bool assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) = 0;

		protected:
			PageMeta(void *physical_start, void *virtual_start);
			uint64_t addressToEntry(void *) const;
	} __attribute__((packed));

	// Four terabytes ought to be enough for anybody.
	struct PageMeta2M: public PageMeta {
		using bitmap_t = int64_t;
		/** Maximum number of pages. */
		int max;
		/** Bits are 0 if the corresponding page is free, 1 if allocated. */
		bitmap_t bitmap[((2 << 20) - sizeof(PageMeta) - sizeof(int)) / sizeof(bitmap_t)];

		PageMeta2M(void *physical_start, void *virtual_start, int max_);
		virtual size_t pageCount() const override;
		virtual size_t pageSize() const override;
		virtual void clear() override;
		virtual int findFree() const override;
		virtual void mark(int index, bool used) override;
		/** Returns true if an entry was assigned. */
		virtual bool assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) override;
	} __attribute__((packed));
}
