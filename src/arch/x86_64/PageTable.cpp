#include "arch/x86_64/mmu.h"
#include "arch/x86_64/PageTable.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "Kernel.h"

namespace x86_64 {
	PageTable::PageTable(uint64_t *entries_, Type type_): entries(entries_), type(type_) {}
	
	void PageTable::clear() {
		memset(entries, 0, PML4_SIZE);
	}

	bool PageTable::assign(uint16_t pml4, uint16_t pdpt, uint16_t pdt, uint16_t pt) {
		return false;
	}

	void PageTable::print() {
		for (int i = 0; i < PML4_SIZE / PML4_ENTRY_SIZE; ++i) {
			const uint64_t pml4e = entries[i];
			if (pml4e) {
				printf("%d: 0x%lx (PML4E)", i, pml4e >> 12 << 12);
				printMeta(pml4e);
				size_t i_shift = (size_t) i << 39;
				printf(" 0x%lx\n", i_shift);
				if (pml4e & MMU_PRESENT) {
					for (int j = 0; j < PDPT_SIZE / PDPT_ENTRY_SIZE; ++j) {
						const uint64_t pdpe = ((uint64_t *) (pml4e >> 12 << 12))[j];
						if (pdpe) {
							printf("  %d: 0x%lx (PDPE)", j, pdpe >> 12 << 12);
							printMeta(pdpe);
							size_t j_shift = i_shift | ((size_t) j << 30);
							printf(" 0x%lx\n", j_shift);
							if (pdpe & MMU_PRESENT) {
								for (int k = 0; k < PAGE_DIRECTORY_SIZE / PAGE_DIRECTORY_ENTRY_SIZE; ++k) {
									const uint64_t pde = ((uint64_t *) (pdpe >> 12 << 12))[k];
									if (pde) {
										printf("    %d: 0x%lx (PDE)", k, pde >> 12 << 12);
										printMeta(pde);
										size_t k_shift = j_shift | ((size_t) k << 21);
										printf(" 0x%lx\n", k_shift);
										if ((pde & MMU_PRESENT) && !(pde & MMU_PDE_TWO_MB)) {
											for (int l = 0; l < PAGE_TABLE_SIZE / PAGE_TABLE_ENTRY_SIZE; ++l) {
												const uint64_t pte = ((uint64_t *) (pde >> 12 << 12))[l];
												if (pte) {
													printf("      %d: 0x%lx", l, pte >> 12 << 12);
													printMeta(pte);
													printf(" 0x%lx\n", k_shift | ((size_t) l << 12));
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void PageTable::printMeta(uint64_t entry) {
		if (entry & MMU_PRESENT)
			printf(" pres");
		if (entry & MMU_WRITABLE)
			printf(" writ");
		if (entry & MMU_USER_MEMORY)
			printf(" user");
		if (entry & MMU_PDE_TWO_MB)
			printf(" 2mb");
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
