#include "arch/x86_64/mmu.h"
#include "arch/x86_64/PageTable.h"
#include "lib/printf.h"
#include "memory/memset.h"

namespace x86_64 {
	PageTable::PageTable(uint64_t *entries_, Type type_): entries(entries_), type(type_) {}
	
	void PageTable::clear() {
		memset(entries, 0, PML4_SIZE);
	}

	void PageTable::print() {
		for (int i = 0; i < PML4_SIZE / PML4_ENTRY_SIZE; ++i) {
			const uint64_t entry = entries[i];
			if (entry) {
				printf("%d: 0x%lx", i, entry >> 12 << 12);
				if (entry & MMU_PRESENT)
					printf(" pres");
				if (entry & MMU_WRITABLE)
					printf(" writ");
				if (entry & MMU_USER_MEMORY)
					printf(" user");
				if (entry & MMU_PDE_TWO_MB)
					printf(" 2mb");
				printf("\n");
			}
		}
	}

	uint64_t PageTable::getPML4E(uint16_t pml4_index) const {
		return entries[pml4_index];
	}

	uint64_t PageTable::getPDPE(uint16_t pml4_index, uint16_t pdpt_index) const {
		const uint64_t pml4e = entries[pml4_index];
		const uint64_t *pdpt = (uint64_t *) (pml4e >> 12);
		return pdpt[pdpt_index];
	}

	uint64_t PageTable::getPDE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const {
		const uint64_t pdpe = getPDPE(pml4_index, pdpt_index);
		const uint64_t *pdt = (uint64_t *) (pdpe >> 12);
		return pdt[pdt_index];
	}

	uint64_t PageTable::getPTE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) const {
		const uint64_t pde = getPDE(pml4_index, pdpt_index, pdt_index);
		const uint64_t *pt = (uint64_t *) (pde >> 12);
		return pt[pt_index];
	}

	uint16_t PageTable::getPML4Meta(uint16_t pml4_index) const {
		return entries[pml4_index] & 0xfff;
	}

	uint16_t PageTable::getPDPTMeta(uint16_t pml4_index, uint16_t pdpt_index) const {
		return getPDPE(pml4_index, pdpt_index) & 0xfff;
	}

	uint16_t PageTable::getPDTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const {
		return getPDE(pml4_index, pdpt_index, pdt_index) & 0xfff;
	}

	uint16_t PageTable::getPTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index,
	                              uint16_t pt_index) const {
		return getPTE(pml4_index, pdpt_index, pdt_index, pt_index) & 0xfff;
	}
}
