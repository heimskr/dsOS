#include "arch/x86_64/PageTable.h"
#include "lib/printf.h"
#include "memory/memset.h"

namespace x86_64 {
	PageTable::PageTable(void **entries_, Type type_): entries(entries_), type(type_) {}
	
	void PageTable::clear() {
		memset(entries, 0, PML4_SIZE * PML4_ENTRY_SIZE);
	}

	void PageTable::print() {
		for (int i = 0; i < PML4_SIZE; ++i)
			if (entries[i])
				printf("%d: 0x%lx\n", i, entries[i]);
	}
}
