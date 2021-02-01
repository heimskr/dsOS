#include "arch/x86_64/mmu.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "ThornUtil.h"
#include "Kernel.h"

namespace x86_64 {
	PageTableWrapper::PageTableWrapper(uint64_t *entries_, Type type_): entries(entries_), type(type_) {}
	
	void PageTableWrapper::clear() {
		memset(entries, 0, PML4_SIZE);
	}

	void PageTableWrapper::print(bool putc, bool show_pdpt, bool show_pdt, PTDisplay pt_display) {
		bool old_putc = printf_putc;
		printf_putc = putc;
		printf("Entries: 0x%lx\n", entries);
		for (int i = 0; i < PML4_SIZE / PML4_ENTRY_SIZE; ++i) {
			const uint64_t &pml4e = entries[i];
			if (pml4e) {
				printf("%d (0x%lx): 0x%lx (PML4E)", i, &pml4e, pml4e & ~0xfffL);
				printMeta(pml4e);
				size_t i_shift = (size_t) i << 39;
				printf(" 0x%lx\n", i_shift);
				if (!Thorn::Util::isCanonical(pml4e))
					printf("  Non-canonical.\n");
				if ((pml4e & MMU_PRESENT) && show_pdpt)
					printPDPT(i_shift, pml4e, show_pdt, pt_display);
			}
		}
		printf_putc = old_putc;
	}

	void PageTableWrapper::printPDPT(size_t i_shift, uint64_t pml4e, bool show_pdt, PTDisplay pt_display) {
		for (int j = 0; j < PDPT_SIZE / PDPT_ENTRY_SIZE; ++j) {
			const uint64_t &pdpe = ((uint64_t *) (pml4e & ~0xfffL))[j];
			if (pdpe) {
				printf("  %d (0x%lx): 0x%lx (PDPE)", j, &pdpe, pdpe & ~0xfffL);
				printMeta(pdpe);
				size_t j_shift = i_shift | ((size_t) j << 30);
				printf(" 0x%lx\n", j_shift);
				if (!Thorn::Util::isCanonical(pdpe))
					printf("    Non-canonical.\n");
				else if ((pdpe & MMU_PRESENT) && show_pdt)
					printPDT(j_shift, pdpe, pt_display);
			}
		}
	}

	void PageTableWrapper::printPDT(size_t j_shift, uint64_t pdpe, PTDisplay pt_display) {
		for (int k = 0; k < PAGE_DIRECTORY_SIZE / PAGE_DIRECTORY_ENTRY_SIZE; ++k) {
			const uint64_t &pde = ((uint64_t *) (pdpe & ~0xfffL))[k];
			if (pde) {
				printf("    %d (0x%lx): 0x%lx (PDE)", k, &pde, pde & ~0xfffL);
				printMeta(pde);
				size_t k_shift = j_shift | ((size_t) k << 21);
				printf(" 0x%lx\n", k_shift);
				if (!Thorn::Util::isCanonical(pde))
					printf("      Non-canonical.\n");
				else if (pt_display == PTDisplay::Full) {
					if ((pde & MMU_PRESENT) && !(pde & MMU_PDE_TWO_MB))
						printPT(k_shift, pde);
				} else if (pt_display == PTDisplay::Condensed)
					printCondensed(k_shift, pde);
			}
		}
	}

	void PageTableWrapper::printPT(size_t k_shift, uint64_t pde) {
		for (int l = 0; l < PAGE_TABLE_SIZE / PAGE_TABLE_ENTRY_SIZE; ++l) {
			const uint64_t &pte = ((uint64_t *) (pde & ~0xfff))[l];
			if (pte) {
				printf("      %d (0x%lx): 0x%lx", l, &pte, pte & ~0xfffL);
				printMeta(pte);
				printf(" 0x%lx\n", k_shift | ((size_t) l << 12));
			}
		}
	}

	void PageTableWrapper::printCondensed(size_t k_shift, uint64_t pde) {
		if (!Thorn::Util::isCanonical(pde)) {
			printf("      Non-canonical.\n");
			return;
		}
		int first = -1, last = -1;
		constexpr int max = PAGE_TABLE_SIZE / PAGE_TABLE_ENTRY_SIZE;
		for (int i = 0; i < max; ++i) {
			const uint64_t pte = ((uint64_t *) (pde & ~0xfffL))[i];
			if (pte) {
				if (first == -1)
					first = i;
				last = i;
			}
		}

		if (first == -1) {
			printf("      %d empty\n", max);
		} else {
			uint64_t first_pte = ((uint64_t *) (pde & ~0xfffL))[first];
			printf("      %d: 0x%lx (PTE)", first, first_pte & ~0xfffL);
			printMeta(first_pte);
			printf(" 0x%lx\n", k_shift | ((size_t) first << 12));
			if (first < last) {
				if (first != last - 1) {
					int nonempty = 0;
					for (int i = first + 1; i < last; ++i)
						if (((uint64_t *) (pde & ~0xfffL))[i])
							++nonempty;
					printf("      ... %d nonempty\n", nonempty);
				}
				uint64_t last_pte = ((uint64_t *) (pde & ~0xfffL))[last];
				printf("      %d: 0x%lx (PTE)", last, last_pte & ~0xfffL);
				printMeta(last_pte);
				printf(" 0x%lx\n", k_shift | ((size_t) last << 12));
			}
		}
	}

	void PageTableWrapper::printMeta(uint64_t entry) {
		if (entry & MMU_PRESENT)
			printf(" pres");
		if (entry & MMU_WRITABLE)
			printf(" writ");
		if (entry & MMU_USER_MEMORY)
			printf(" user");
		if (entry & MMU_PDE_TWO_MB)
			printf(" 2mb");
	}

	uint64_t PageTableWrapper::getPML4E(uint16_t pml4_index) const {
		return entries[pml4_index];
	}

	uint64_t PageTableWrapper::getPDPE(uint16_t pml4_index, uint16_t pdpt_index) const {
		const uint64_t pml4e = entries[pml4_index];
		const uint64_t *pdpt = (uint64_t *) (pml4e >> 12);
		return pdpt[pdpt_index];
	}

	uint64_t PageTableWrapper::getPDE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const {
		const uint64_t pdpe = getPDPE(pml4_index, pdpt_index);
		const uint64_t *pdt = (uint64_t *) (pdpe >> 12);
		return pdt[pdt_index];
	}

	uint64_t PageTableWrapper::getPTE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) const {
		const uint64_t pde = getPDE(pml4_index, pdpt_index, pdt_index);
		const uint64_t *pt = (uint64_t *) (pde >> 12);
		return pt[pt_index];
	}

	uint16_t PageTableWrapper::getPML4Meta(uint16_t pml4_index) const {
		return entries[pml4_index] & 0xfff;
	}

	uint16_t PageTableWrapper::getPDPTMeta(uint16_t pml4_index, uint16_t pdpt_index) const {
		return getPDPE(pml4_index, pdpt_index) & 0xfff;
	}

	uint16_t PageTableWrapper::getPDTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const {
		return getPDE(pml4_index, pdpt_index, pdt_index) & 0xfff;
	}

	uint16_t PageTableWrapper::getPTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index,
	                              uint16_t pt_index) const {
		return getPTE(pml4_index, pdpt_index, pdt_index, pt_index) & 0xfff;
	}
}
