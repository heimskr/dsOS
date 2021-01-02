#include "arch/x86_64/PageMeta.h"
#include "lib/printf.h"
#include "memory/memset.h"

namespace x86_64 {
	PageMeta::PageMeta(void *physical_start, void *virtual_start):
		physicalStart(physical_start), virtualStart(virtual_start) {}

	void * PageMeta::findFreePhysicalAddress() const {
		int free_index = findFree();
		if (free_index == -1)
			return nullptr;
		return (void *) ((uintptr_t) physicalStart + free_index * pageSize());
	}
	
	PageMeta2M::PageMeta2M(void *physical_start, void *virtual_start, int max_):
		PageMeta(physical_start, virtual_start), max(max_) {}

	size_t PageMeta2M::pageCount() const {
		return sizeof(bitmap);
	}

	size_t PageMeta2M::pageSize() const {
		return 2 << 20;
	}

	void PageMeta2M::clear() {
		memset(bitmap, 0, sizeof(bitmap));
	}

	int PageMeta2M::findFree() const {
		for (int i = 0; i < max / sizeof(bitmap_t); ++i)
			if (bitmap[i] != -1L)
				for (unsigned int j = 0; j < sizeof(bitmap_t); ++j)
					if ((bitmap[i] & (1 << j)) == 0)
						return (i * sizeof(bitmap_t)) + j;
		return -1;
	}

	void PageMeta2M::mark(int index, bool used) {
		if (used)
			bitmap[index / sizeof(bitmap_t)] |= 1 << (index % sizeof(bitmap_t));
		else
			bitmap[index / sizeof(bitmap_t)] &= ~(1 << (index % sizeof(bitmap_t)));
	}
}
