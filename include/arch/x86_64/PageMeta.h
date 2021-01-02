#pragma once

#include <stddef.h>

#include "mmu.h"

namespace x86_64 {
	class PageMeta {
		public:
			void *physicalStart;
			void *virtualStart;
			virtual size_t pageCount() const = 0;
			virtual void clear() = 0;

		protected:
			PageMeta(void *physical_start, void *virtual_start);
	} __attribute__((packed));

	// Four terabytes ought to be enough for anybody.
	struct PageMeta2M: public PageMeta {
		int max;
		bool allocated[(2 << 20) - sizeof(PageMeta) - sizeof(int)];

		PageMeta2M(void *physical_start, void *virtual_start, int max_);
		virtual size_t pageCount() const override;
		virtual void clear() override;
	} __attribute__((packed));
}
