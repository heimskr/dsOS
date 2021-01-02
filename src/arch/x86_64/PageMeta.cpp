#include "arch/x86_64/PageMeta.h"
#include "lib/printf.h"
#include "memory/memset.h"

namespace x86_64 {
	PageMeta::PageMeta(void *physical_start, void *virtual_start):
		physicalStart(physical_start), virtualStart(virtual_start) {}
	
	PageMeta2M::PageMeta2M(void *physical_start, void *virtual_start, int max_):
		PageMeta(physical_start, virtual_start), max(max_) {}

	size_t PageMeta2M::pageCount() const {
		return sizeof(allocated);
	}

	void PageMeta2M::clear() {
		memset(allocated, 0, sizeof(allocated));
	}
}
